from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..core.context import Context
from .tidy_paths import TidyPaths
from .tidy_queue import load_manifest, write_manifest_from_entries
from .tidy_runtime import latest_verify_result_mtime, latest_verify_succeeded


@dataclass(frozen=True)
class CleanResult:
    archived_count: int
    cleaned_task_ids: list[str]
    touched_files: list[Path]


def clean_tasks(
    ctx: Context,
    paths: TidyPaths,
    *,
    batch_id: str | None = None,
    task_ids: list[str] | None = None,
    strict: bool = False,
    cluster_by_file: bool = False,
) -> CleanResult:
    manifest = load_manifest(paths.tasks_manifest)
    entries = list(manifest.get("tasks", []))
    normalized_batch = (batch_id or "").strip()
    normalized_task_ids = {
        str(task_id).strip().zfill(3)
        for task_id in (task_ids or [])
        if str(task_id).strip()
    }

    if not normalized_batch and not normalized_task_ids:
        raise ValueError("clean requires --batch-id or explicit task ids.")

    selected_entries = [
        entry
        for entry in entries
        if _matches_task(entry, normalized_batch, normalized_task_ids)
    ]
    if cluster_by_file and selected_entries:
        clustered_files = {
            _path_key(_entry_source_path(ctx, entry))
            for entry in selected_entries
        }
        selected_entries = [
            entry
            for entry in entries
            if _matches_batch(entry, normalized_batch)
            and _path_key(_entry_source_path(ctx, entry)) in clustered_files
        ]

    if strict:
        verify_ok, verify_reason = latest_verify_succeeded(paths)
        if not verify_ok:
            raise ValueError(
                "strict clean requires a successful verify result "
                f"({verify_reason})."
            )
        verify_mtime = latest_verify_result_mtime(paths)
        if verify_mtime is None:
            raise ValueError("strict clean requires a readable verify result timestamp.")
    else:
        verify_mtime = None

    selected_keys = {
        (str(entry.get("task_id", "")), str(entry.get("batch_id", "")))
        for entry in selected_entries
    }
    remaining_entries: list[dict] = []
    cleaned_task_ids: list[str] = []
    touched_files: list[Path] = []
    archived_count = 0

    for entry in entries:
        key = (str(entry.get("task_id", "")), str(entry.get("batch_id", "")))
        if key not in selected_keys:
            remaining_entries.append(entry)
            continue

        log_path = _entry_log_path(paths, entry)
        if not log_path.exists():
            continue

        if strict:
            strict_ok, strict_reason = _strict_task_guard(
                ctx=ctx,
                entry=entry,
                log_path=log_path,
                verify_result_mtime=verify_mtime,
            )
            if not strict_ok:
                raise ValueError(
                    f"strict clean rejected task_{entry.get('task_id')}: {strict_reason}"
                )

        relative_path = log_path.relative_to(paths.tasks_dir)
        archive_path = paths.tasks_done_dir / relative_path
        archive_path.parent.mkdir(parents=True, exist_ok=True)
        if archive_path.exists():
            archive_path.unlink()
        log_path.replace(archive_path)
        archived_count += 1

        task_id = str(entry.get("task_id", "")).zfill(3)
        if task_id and task_id not in cleaned_task_ids:
            cleaned_task_ids.append(task_id)

        source_path = _entry_source_path(ctx, entry)
        if _path_key(source_path) not in {_path_key(item) for item in touched_files}:
            touched_files.append(source_path)

    write_manifest_from_entries(
        tasks_manifest=paths.tasks_manifest,
        tasks_summary_path=paths.tasks_summary,
        entries=remaining_entries,
        batch_size=int(manifest.get("batch_size", ctx.config.tidy.batch_size) or 0)
        or ctx.config.tidy.batch_size,
    )
    _cleanup_empty_batch_dirs(paths.tasks_dir)

    return CleanResult(
        archived_count=archived_count,
        cleaned_task_ids=cleaned_task_ids,
        touched_files=touched_files,
    )


def _matches_task(entry: dict, normalized_batch: str, normalized_task_ids: set[str]) -> bool:
    if normalized_batch and str(entry.get("batch_id", "")) != normalized_batch:
        return False
    if normalized_task_ids:
        return str(entry.get("task_id", "")).zfill(3) in normalized_task_ids
    return bool(normalized_batch)


def _matches_batch(entry: dict, normalized_batch: str) -> bool:
    if not normalized_batch:
        return True
    return str(entry.get("batch_id", "")) == normalized_batch


def _strict_task_guard(
    *,
    ctx: Context,
    entry: dict,
    log_path: Path,
    verify_result_mtime: float | None,
) -> tuple[bool, str]:
    if verify_result_mtime is None:
        return False, "verify result timestamp unavailable"
    try:
        log_mtime = log_path.stat().st_mtime
    except OSError:
        return False, "task log timestamp unavailable"
    if verify_result_mtime <= log_mtime:
        return False, "verify result is older than the task log"

    source_path = _entry_source_path(ctx, entry)
    if not source_path.exists():
        return False, f"source file missing: {source_path}"
    try:
        source_mtime = source_path.stat().st_mtime
    except OSError:
        return False, "source file timestamp unavailable"
    if verify_result_mtime <= source_mtime:
        return False, "verify result is older than the source file"
    return True, "ok"


def _entry_log_path(paths: TidyPaths, entry: dict) -> Path:
    content_path = str(entry.get("content_path", "")).strip()
    if not content_path:
        return paths.tasks_dir / str(entry.get("batch_id", "")) / (
            f"task_{str(entry.get('task_id', '')).zfill(3)}.log"
        )
    return paths.tasks_dir.parent / content_path


def _entry_source_path(ctx: Context, entry: dict) -> Path:
    source_raw = str(entry.get("source_file", "")).strip()
    source_path = Path(source_raw)
    if source_path.is_absolute():
        return source_path
    return (ctx.repo_root / source_path).resolve()


def _cleanup_empty_batch_dirs(tasks_dir: Path) -> None:
    for batch_dir in sorted(tasks_dir.glob("batch_*"), reverse=True):
        if batch_dir.is_dir() and not any(batch_dir.iterdir()):
            batch_dir.rmdir()


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()
