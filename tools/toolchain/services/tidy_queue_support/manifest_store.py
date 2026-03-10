from __future__ import annotations

import json
import shutil
from pathlib import Path

from ..timestamps import utc_now_iso
from .summary import _write_markdown_summary
from .utils import _batch_num, _norm, _relpath


def rewrite_pending_tasks(
    ctx,
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
            "safe_fix_checks_present": list(task.get("safe_fix_checks_present", [])),
            "suppression_candidates_present": list(task.get("suppression_candidates_present", [])),
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
                "safe_fix_checks_present": set(),
                "suppression_candidates_present": set(),
            },
        )
        summary["task_count"] += 1
        summary["checks"].update(task["checks"])
        summary["files"].add(task["source_file"])
        summary["primary_fix_strategy"].add(task["primary_fix_strategy"])
        summary["safe_fix_checks_present"].update(task.get("safe_fix_checks_present", []))
        summary["suppression_candidates_present"].update(
            task.get("suppression_candidates_present", [])
        )

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
                "safe_fix_checks_present": set(),
                "suppression_candidates_present": set(),
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
        summary["safe_fix_checks_present"].update(entry.get("safe_fix_checks_present", []))
        summary["suppression_candidates_present"].update(
            entry.get("suppression_candidates_present", [])
        )

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
                "safe_fix_checks_present": sorted(item["safe_fix_checks_present"]),
                "suppression_candidates_present": sorted(item["suppression_candidates_present"]),
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


def remove_tasks_for_files(
    tasks_dir: Path, tasks_manifest: Path, source_files: set[str]
) -> list[dict]:
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
