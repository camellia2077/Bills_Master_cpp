from __future__ import annotations

from pathlib import Path

from ...core.context import Context
from ..tidy_paths import TidyPaths
from ..tidy_queue import load_manifest
from .state_store import load_batch_runtime_state


def resolve_batch_source_files(
    ctx: Context,
    paths: TidyPaths,
    *,
    batch_id: str,
) -> list[Path]:
    normalized_batch = batch_id.strip()
    manifest = load_manifest(paths.tasks_manifest)
    files: list[Path] = []
    seen: set[str] = set()
    for task in manifest.get("tasks", []):
        if str(task.get("batch_id", "")).strip() != normalized_batch:
            continue
        raw = str(task.get("source_file", "")).strip()
        if not raw:
            continue
        path = _resolve_source_path(ctx, raw)
        key = _path_key(path)
        if key in seen:
            continue
        seen.add(key)
        files.append(path)
    if files:
        return files

    batch_state = load_batch_runtime_state(paths, normalized_batch)
    for raw in batch_state.get("source_files", []):
        path = _resolve_source_path(ctx, str(raw))
        key = _path_key(path)
        if key in seen:
            continue
        seen.add(key)
        files.append(path)
    return files


def extract_task_source_files(task_path: Path) -> list[Path]:
    files: list[Path] = []
    try:
        lines = task_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return files
    for line in lines:
        if not line.startswith("File: "):
            continue
        raw = line[len("File: ") :].strip()
        if not raw:
            continue
        path = Path(raw)
        if path not in files:
            files.append(path)
    return files


def load_pending_tasks_with_content(tasks_root: Path, manifest_path: Path) -> list[dict]:
    manifest = load_manifest(manifest_path)
    tasks: list[dict] = []
    for item in manifest.get("tasks", []):
        content_path = tasks_root.parent / str(item.get("content_path", ""))
        if not content_path.exists():
            continue
        try:
            content = content_path.read_text(encoding="utf-8")
        except OSError:
            continue
        tasks.append(
            {
                "task_id": str(item.get("task_id", "")),
                "batch_id": str(item.get("batch_id", "")),
                "content": content,
                "size": int(item.get("size", len(content))),
                "diagnostics": [{}] * int(item.get("diagnostic_count", 0)),
                "source_file": str(item.get("source_file", "")),
                "checks": list(item.get("checks", [])),
                "primary_fix_strategy": str(item.get("primary_fix_strategy", "manual_only")),
                "safe_fix_checks_present": list(item.get("safe_fix_checks_present", [])),
                "suppression_candidates_present": list(
                    item.get("suppression_candidates_present", [])
                ),
                "score": float(item.get("score", 999.0)),
            }
        )
    return tasks


def _resolve_source_path(ctx: Context, raw: str) -> Path:
    candidate = Path(raw)
    if candidate.is_absolute():
        return candidate
    return (ctx.repo_root / candidate).resolve()


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()
