from __future__ import annotations

from pathlib import Path

from ..core.context import Context
from ..services.tidy_paths import TidyPaths, resolve_tidy_paths
from ..services.tidy_queue import max_indices, rewrite_pending_tasks, split_log_to_tasks
from ..services.tidy_runtime import (
    copy_build_log_to_raw,
    extract_task_source_files,
    write_latest_tidy_summary,
    write_tidy_result,
)
from ..services.tidy_state import set_batch_status
from .common import normalize_forwarded_args, resolve_keep_going


def run_tidy_build(
    ctx: Context,
    *,
    build_dir_name: str = "build_tidy",
    forwarded: list[str] | None = None,
    keep_going: bool | None = None,
    stage: str = "tidy",
) -> tuple[int, TidyPaths]:
    if build_dir_name.strip() and build_dir_name.strip() != "build_tidy":
        print(
            f"[WARN] bills_tracer tidy currently uses `build_tidy`; "
            f"requested `{build_dir_name}` will be ignored."
        )

    paths = resolve_tidy_paths(ctx, "build_tidy")
    command = [
        ctx.python_executable,
        str(ctx.build_entry("build_bills_master.py")),
        "tidy",
    ]
    normalized = normalize_forwarded_args(forwarded or [])
    if resolve_keep_going(ctx, keep_going):
        command.extend(["--", "-k", "0", *normalized])
    else:
        command.extend(normalized)

    result = ctx.process_runner.run(command, cwd=ctx.repo_root)
    copied = copy_build_log_to_raw(paths)
    write_latest_tidy_summary(
        paths,
        stage=stage,
        status="ok" if result.returncode == 0 else "failed",
        exit_code=result.returncode,
        extra={
            "build_dir": str(paths.build_dir),
            "build_log_copied": copied,
            "keep_going": resolve_keep_going(ctx, keep_going),
        },
    )
    write_tidy_result(
        ctx,
        paths,
        stage=stage,
        status="ok" if result.returncode == 0 else "failed",
        exit_code=result.returncode,
    )
    return result.returncode, paths


def split_captured_log(
    ctx: Context,
    *,
    paths: TidyPaths,
    max_lines: int | None = None,
    max_diags: int | None = None,
    batch_size: int | None = None,
    preserve_history: bool = True,
    stage: str = "tidy-split",
) -> tuple[int, dict]:
    if not paths.latest_build_log.exists():
        print(f"[ERROR] Missing raw tidy log: {paths.latest_build_log}")
        write_latest_tidy_summary(
            paths,
            stage=stage,
            status="failed",
            exit_code=1,
            extra={"reason": "missing_raw_build_log"},
        )
        write_tidy_result(ctx, paths, stage=stage, status="failed", exit_code=1)
        return 1, {}

    try:
        log_content = paths.latest_build_log.read_text(
            encoding="utf-8",
            errors="replace",
        )
        tasks = split_log_to_tasks(
            ctx,
            log_content=log_content,
            max_lines=max_lines,
            max_diags=max_diags,
        )
    except ValueError as exc:
        print(f"[ERROR] {exc}")
        write_latest_tidy_summary(
            paths,
            stage=stage,
            status="failed",
            exit_code=2,
            extra={"reason": str(exc)},
        )
        write_tidy_result(ctx, paths, stage=stage, status="failed", exit_code=2)
        return 2, {}

    start_task_index = 1
    start_batch_index = 1
    if preserve_history:
        max_task_id, max_batch_id = max_indices(paths.tasks_manifest, paths.tasks_done_dir)
        start_task_index = max_task_id + 1 if max_task_id else 1
        start_batch_index = max_batch_id + 1 if max_batch_id else 1

    manifest = rewrite_pending_tasks(
        ctx,
        tasks=tasks,
        tasks_dir=paths.tasks_dir,
        tasks_manifest=paths.tasks_manifest,
        tasks_summary_path=paths.tasks_summary,
        batch_size=batch_size,
        start_task_index=start_task_index,
        start_batch_index=start_batch_index,
        reset_directory=True,
    )
    sync_open_batch_status(paths, manifest)
    write_latest_tidy_summary(
        paths,
        stage=stage,
        status="ok",
        exit_code=0,
        extra={
            "task_count": int(manifest.get("task_count", 0)),
            "batch_count": int(manifest.get("batch_count", 0)),
            "preserve_history": preserve_history,
        },
    )
    write_tidy_result(ctx, paths, stage=stage, status="ok", exit_code=0)
    return 0, manifest


def sync_open_batch_status(paths: TidyPaths, manifest: dict) -> None:
    for batch in manifest.get("batches", []):
        batch_id = str(batch.get("batch_id", "")).strip()
        if not batch_id:
            continue
        set_batch_status(
            paths.batch_status_path,
            batch_id,
            "open",
            task_count=int(batch.get("task_count", 0)),
            checks=list(batch.get("checks", [])),
            files=list(batch.get("files", [])),
            primary_fix_strategy=list(batch.get("primary_fix_strategy", [])),
        )


def detect_auto_full_reasons_from_text(ctx: Context, text: str) -> list[str]:
    normalized = text.lower()
    reasons: list[str] = []
    if ctx.config.tidy.auto_full_on_no_such_file and "no such file" in normalized:
        reasons.append("no_such_file")
    if ctx.config.tidy.auto_full_on_glob_mismatch and (
        "glob mismatch" in normalized or "glob does not match" in normalized
    ):
        reasons.append("glob_mismatch")
    return reasons


def collect_touched_files_from_done_batch(paths: TidyPaths, batch_id: str) -> list[Path]:
    batch_dir = paths.tasks_done_dir / batch_id
    files: list[Path] = []
    seen: set[str] = set()
    if not batch_dir.exists():
        return files
    for task_path in sorted(batch_dir.glob("task_*.log")):
        for source_path in extract_task_source_files(task_path):
            key = normalize_path_key(source_path)
            if key in seen:
                continue
            seen.add(key)
            files.append(source_path)
    return files


def normalize_path_key(path: Path | str) -> str:
    return str(path).replace("\\", "/").lower()
