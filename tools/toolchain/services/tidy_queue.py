from __future__ import annotations

import json
import shutil
from pathlib import Path

from ..core.context import Context
from .fix_strategy import resolve_primary_strategy
from .task_sorter import calculate_priority_score
from .tidy_log_parser import extract_diagnostics, generate_text_summary, group_sections
from .timestamps import utc_now_iso


def split_log_to_tasks(
    ctx: Context,
    *,
    log_content: str,
    max_lines: int | None = None,
    max_diags: int | None = None,
) -> list[dict]:
    effective_max_lines = ctx.config.tidy.max_lines if max_lines is None else max_lines
    effective_max_diags = ctx.config.tidy.max_diags if max_diags is None else max_diags
    if effective_max_lines <= 0 or effective_max_diags <= 0:
        raise ValueError("max_lines and max_diags must be > 0")

    tasks: list[dict] = []
    for section in group_sections(log_content.splitlines()):
        tasks.extend(
            _process_section(
                section,
                max_lines=effective_max_lines,
                max_diags=effective_max_diags,
                strategy_cfg=ctx.config.tidy.fix_strategy,
            )
        )
    tasks.sort(key=lambda item: (item["score"], item["size"]))
    return tasks


def rewrite_pending_tasks(
    ctx: Context,
    *,
    tasks: list[dict],
    tasks_dir: Path,
    tasks_manifest: Path,
    tasks_summary_path: Path,
    batch_size: int | None = None,
    start_task_index: int = 1,
    start_batch_index: int = 1,
    reset_directory: bool = True,
) -> dict:
    effective_batch_size = ctx.config.tidy.batch_size if batch_size is None else batch_size
    if effective_batch_size <= 0:
        raise ValueError("batch_size must be > 0")

    if reset_directory and tasks_dir.exists():
        shutil.rmtree(tasks_dir)
    tasks_dir.mkdir(parents=True, exist_ok=True)

    manifest_tasks: list[dict] = []
    batch_summaries: dict[str, dict] = {}
    for offset, task in enumerate(tasks, start=0):
        task_number = start_task_index + offset
        batch_number = start_batch_index + (offset // effective_batch_size)
        task_id = f"{task_number:03d}"
        batch_id = f"batch_{batch_number:03d}"
        batch_dir = tasks_dir / batch_id
        batch_dir.mkdir(parents=True, exist_ok=True)
        task_path = batch_dir / f"task_{task_id}.log"
        task_path.write_text(task["content"], encoding="utf-8")
        entry = {
            "task_id": task_id,
            "batch_id": batch_id,
            "source_file": task["source_file"],
            "score": task["score"],
            "size": task["size"],
            "checks": task["checks"],
            "diagnostic_count": len(task["diagnostics"]),
            "primary_fix_strategy": task["primary_fix_strategy"],
            "content_path": _relpath(tasks_dir.parent, task_path),
        }
        manifest_tasks.append(entry)
        summary = batch_summaries.setdefault(
            batch_id,
            {
                "batch_id": batch_id,
                "task_count": 0,
                "checks": set(),
                "files": set(),
                "primary_fix_strategy": set(),
            },
        )
        summary["task_count"] += 1
        summary["checks"].update(task["checks"])
        summary["files"].add(task["source_file"])
        summary["primary_fix_strategy"].add(task["primary_fix_strategy"])

    del batch_summaries
    return write_manifest_from_entries(
        tasks_manifest=tasks_manifest,
        tasks_summary_path=tasks_summary_path,
        entries=manifest_tasks,
        batch_size=effective_batch_size,
    )


def write_manifest_from_entries(
    *,
    tasks_manifest: Path,
    tasks_summary_path: Path,
    entries: list[dict],
    batch_size: int,
) -> dict:
    sorted_entries = sorted(
        entries,
        key=lambda item: (
            _batch_num(str(item.get("batch_id", ""))),
            int(str(item.get("task_id", "0")) or "0"),
        ),
    )
    batch_summaries: dict[str, dict] = {}
    for entry in sorted_entries:
        batch_id = str(entry.get("batch_id", ""))
        summary = batch_summaries.setdefault(
            batch_id,
            {
                "batch_id": batch_id,
                "task_count": 0,
                "checks": set(),
                "files": set(),
                "primary_fix_strategy": set(),
            },
        )
        summary["task_count"] += 1
        summary["checks"].update(entry.get("checks", []))
        source_file = str(entry.get("source_file", "")).strip()
        if source_file:
            summary["files"].add(source_file)
        strategy = str(entry.get("primary_fix_strategy", "")).strip()
        if strategy:
            summary["primary_fix_strategy"].add(strategy)

    payload = {
        "generated_at": utc_now_iso(),
        "task_count": len(sorted_entries),
        "batch_count": len(batch_summaries),
        "batch_size": int(batch_size),
        "tasks": sorted_entries,
        "batches": [
            {
                "batch_id": batch_id,
                "task_count": item["task_count"],
                "checks": sorted(item["checks"]),
                "files": sorted(item["files"]),
                "primary_fix_strategy": sorted(item["primary_fix_strategy"]),
            }
            for batch_id, item in sorted(batch_summaries.items())
        ],
    }
    tasks_manifest.parent.mkdir(parents=True, exist_ok=True)
    tasks_manifest.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    _write_markdown_summary(sorted_entries, tasks_summary_path)
    return payload


def load_manifest(tasks_manifest: Path) -> dict:
    if not tasks_manifest.exists():
        return {
            "generated_at": None,
            "task_count": 0,
            "batch_count": 0,
            "batch_size": 0,
            "tasks": [],
            "batches": [],
        }
    try:
        payload = json.loads(tasks_manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {
            "generated_at": None,
            "task_count": 0,
            "batch_count": 0,
            "batch_size": 0,
            "tasks": [],
            "batches": [],
        }
    if not isinstance(payload, dict):
        return {
            "generated_at": None,
            "task_count": 0,
            "batch_count": 0,
            "batch_size": 0,
            "tasks": [],
            "batches": [],
        }
    payload.setdefault("tasks", [])
    payload.setdefault("batches", [])
    return payload


def remove_tasks_for_files(tasks_dir: Path, tasks_manifest: Path, source_files: set[str]) -> list[dict]:
    manifest = load_manifest(tasks_manifest)
    remaining_tasks: list[dict] = []
    removed_tasks: list[dict] = []
    normalized_source_files = {_norm(path) for path in source_files}
    for task in manifest.get("tasks", []):
        source_file = _norm(str(task.get("source_file", "")))
        if source_file in normalized_source_files:
            removed_tasks.append(task)
            content_relpath = str(task.get("content_path", ""))
            if content_relpath:
                candidate = tasks_dir.parent / content_relpath
                if candidate.exists():
                    candidate.unlink()
            continue
        remaining_tasks.append(task)
    # cleanup empty batch dirs
    for batch_dir in sorted(tasks_dir.glob("batch_*"), reverse=True):
        if batch_dir.is_dir() and not any(batch_dir.iterdir()):
            batch_dir.rmdir()
    write_manifest_from_entries(
        tasks_manifest=tasks_manifest,
        tasks_summary_path=tasks_dir / "tasks_summary.md",
        entries=remaining_tasks,
        batch_size=int(manifest.get("batch_size", 0) or 0),
    )
    return removed_tasks


def load_batch_tasks(tasks_manifest: Path, batch_id: str) -> list[dict]:
    manifest = load_manifest(tasks_manifest)
    return [task for task in manifest.get("tasks", []) if str(task.get("batch_id")) == batch_id]


def next_open_batch(tasks_manifest: Path) -> str | None:
    manifest = load_manifest(tasks_manifest)
    batches = sorted({str(task.get("batch_id")) for task in manifest.get("tasks", []) if str(task.get("batch_id", "")).startswith("batch_")})
    return batches[0] if batches else None


def max_indices(tasks_manifest: Path, tasks_done_dir: Path) -> tuple[int, int]:
    task_ids: list[int] = []
    batch_ids: list[int] = []
    manifest = load_manifest(tasks_manifest)
    for task in manifest.get("tasks", []):
        task_ids.append(int(str(task.get("task_id", "0"))))
        batch_ids.append(_batch_num(str(task.get("batch_id", ""))))
    for task_path in tasks_done_dir.rglob("task_*.log"):
        try:
            task_ids.append(int(task_path.stem.replace("task_", "")))
        except ValueError:
            pass
        batch_ids.append(_batch_num(task_path.parent.name))
    return (max(task_ids) if task_ids else 0, max(batch_ids) if batch_ids else 0)


def _process_section(
    section_lines: list[str],
    *,
    max_lines: int,
    max_diags: int,
    strategy_cfg,
) -> list[dict]:
    diagnostics = extract_diagnostics(section_lines)
    if not diagnostics:
        return []

    tasks: list[dict] = []
    current_batch: list[dict] = []
    batch_lines = 0

    def finalize(batch: list[dict]) -> None:
        if not batch:
            return
        source_file = str(batch[0]["file"])
        checks = sorted({str(item["check"]) for item in batch})
        summary = generate_text_summary(batch)
        original_lines: list[str] = []
        for diagnostic in batch:
            original_lines.extend(diagnostic["lines"])
        content = (
            f"File: {source_file}\n"
            + "=" * 60
            + "\n"
            + summary
            + "\n".join(original_lines)
            + ("\n" if original_lines else "")
        )
        tasks.append(
            {
                "content": content,
                "size": len(content),
                "diagnostics": batch,
                "source_file": source_file,
                "checks": checks,
                "primary_fix_strategy": resolve_primary_strategy(checks, strategy_cfg),
                "score": calculate_priority_score(batch, source_file),
            }
        )

    for diagnostic in diagnostics:
        diagnostic_line_count = len(diagnostic["lines"])
        if current_batch and (
            len(current_batch) >= max_diags or batch_lines + diagnostic_line_count > max_lines
        ):
            finalize(current_batch)
            current_batch = []
            batch_lines = 0
        current_batch.append(diagnostic)
        batch_lines += diagnostic_line_count

    finalize(current_batch)
    return tasks


def _write_markdown_summary(tasks: list[dict], out_path: Path) -> None:
    lines = [
        "# Clang-Tidy Tasks Summary\n",
        "| ID | Batch | File | Difficulty Score | Warning Types | Fix Strategy |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for task in tasks:
        lines.append(
            "| {task_id} | {batch_id} | {source_file} | {score:.2f} | {checks} | {strategy} |".format(
                task_id=task["task_id"],
                batch_id=task["batch_id"],
                source_file=task["source_file"],
                score=float(task["score"]),
                checks=", ".join(task["checks"]),
                strategy=task["primary_fix_strategy"],
            )
        )
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")


def _relpath(base_dir: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(base_dir.resolve()))
    except ValueError:
        return str(path)


def _norm(text: str) -> str:
    return text.replace("\\", "/").lower()


def _batch_num(batch_id: str) -> int:
    if batch_id.startswith("batch_"):
        try:
            return int(batch_id.replace("batch_", ""))
        except ValueError:
            return 0
    return 0
