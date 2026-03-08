from __future__ import annotations

from pathlib import Path

from ..core.context import Context
from ..services.clang_tidy_runner import run_clang_tidy
from ..services.incremental_mapper import load_compile_units, resolve_incremental_files
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import max_indices, rewrite_pending_tasks, split_log_to_tasks
from ..services.tidy_runtime import (
    load_pending_tasks_with_content,
    write_latest_tidy_summary,
    write_tidy_result,
)
from ..services.tidy_state import load_refresh_state, update_batch_state, update_refresh_state
from ..services.timestamps import utc_now_iso
from .common import resolve_keep_going
from .tidy_support import (
    collect_touched_files_from_done_batch,
    detect_auto_full_reasons_from_text,
    normalize_path_key,
    run_tidy_build,
    split_captured_log,
    sync_open_batch_status,
)


def execute_refresh(
    ctx: Context,
    *,
    batch_id: str = "",
    full_every: int | None = None,
    force_full: bool = False,
    final_full: bool = False,
    dry_run: bool = False,
    keep_going: bool | None = None,
    neighbor_scope: str | None = None,
) -> int:
    paths = resolve_tidy_paths(ctx)
    normalized_batch = batch_id.strip()
    effective_full_every = ctx.config.tidy.full_every if full_every is None else int(full_every)
    effective_keep_going = resolve_keep_going(ctx, keep_going)
    effective_neighbor_scope = neighbor_scope or ctx.config.tidy.neighbor_scope or "none"

    state = load_refresh_state(paths.refresh_state_path)
    processed_batches = [
        str(item) for item in state.get("processed_batches", []) if str(item).strip()
    ]
    already_processed = normalized_batch in processed_batches if normalized_batch else False
    if normalized_batch and not already_processed:
        processed_batches.append(normalized_batch)
        state["processed_batches"] = processed_batches[-200:]
        state["batches_since_full"] = int(state.get("batches_since_full", 0)) + 1
    if normalized_batch:
        state["last_batch"] = normalized_batch

    full_reasons: list[str] = []
    if force_full:
        full_reasons.append("force_full")
    if final_full:
        full_reasons.append("final_full")
    if (
        normalized_batch
        and not already_processed
        and effective_full_every > 0
        and int(state.get("batches_since_full", 0)) >= effective_full_every
    ):
        full_reasons.append(f"cadence_{effective_full_every}")

    build_log_text = _read_text_if_exists(paths.latest_build_log)
    _append_unique(full_reasons, detect_auto_full_reasons_from_text(ctx, build_log_text))

    touched_files = (
        collect_touched_files_from_done_batch(paths, normalized_batch) if normalized_batch else []
    )
    incremental_files: list[Path] = []
    if not full_reasons and normalized_batch:
        if not touched_files:
            full_reasons.append("no_touched_files")
        elif not paths.compile_commands_path.exists():
            full_reasons.append("missing_compile_commands")
        else:
            compile_units = load_compile_units(paths.compile_commands_path)
            incremental_files = resolve_incremental_files(
                touched_files,
                compile_units,
                ctx.repo_root,
                effective_neighbor_scope,
            )
            if not incremental_files:
                full_reasons.append("empty_incremental_scope")

    if dry_run:
        print(
            "--- tidy-refresh preview:"
            f" batch={normalized_batch or '<none>'}"
            f", full_reasons={','.join(full_reasons) or '<incremental>'}"
            f", touched={len(touched_files)}"
            f", incremental={len(incremental_files)}"
            f", keep_going={effective_keep_going}"
            f", neighbor_scope={effective_neighbor_scope}"
        )
        return 0

    if not full_reasons and normalized_batch:
        incremental_log = paths.refresh_dir / f"{normalized_batch}_incremental.log"
        returncode = run_clang_tidy(
            ctx,
            compile_commands_dir=paths.compile_commands_path.parent,
            files=incremental_files,
            output_log=incremental_log,
            fix=False,
        )
        incremental_text = _read_text_if_exists(incremental_log)
        if returncode != 0:
            auto_full_reasons = detect_auto_full_reasons_from_text(ctx, incremental_text)
            if not auto_full_reasons:
                update_batch_state(
                    paths.batch_state_path,
                    batch_id=normalized_batch,
                    last_refresh_ok=False,
                )
                write_latest_tidy_summary(
                    paths,
                    stage="tidy-refresh",
                    status="failed",
                    exit_code=returncode,
                    extra={
                        "mode": "incremental",
                        "batch_id": normalized_batch,
                        "incremental_log": str(incremental_log),
                    },
                )
                write_tidy_result(
                    ctx,
                    paths,
                    stage="tidy-refresh",
                    status="failed",
                    exit_code=returncode,
                )
                return returncode
            _append_unique(full_reasons, auto_full_reasons)

        if not full_reasons:
            new_tasks = split_log_to_tasks(ctx, log_content=incremental_text)
            pending_tasks = load_pending_tasks_with_content(
                paths.tasks_dir,
                paths.tasks_manifest,
            )
            impacted_sources = {
                normalize_path_key(path) for path in [*touched_files, *incremental_files]
            }
            remaining_tasks = [
                task
                for task in pending_tasks
                if normalize_path_key(task.get("source_file", "")) not in impacted_sources
            ]
            next_task_index, next_batch_index = max_indices(
                paths.tasks_manifest,
                paths.tasks_done_dir,
            )
            manifest = rewrite_pending_tasks(
                ctx,
                tasks=[*remaining_tasks, *new_tasks],
                tasks_dir=paths.tasks_dir,
                tasks_manifest=paths.tasks_manifest,
                tasks_summary_path=paths.tasks_summary,
                batch_size=ctx.config.tidy.batch_size,
                start_task_index=next_task_index + 1 if next_task_index else 1,
                start_batch_index=next_batch_index + 1 if next_batch_index else 1,
                reset_directory=True,
            )
            sync_open_batch_status(paths, manifest)
            state["last_full_reason"] = ""
            update_refresh_state(paths.refresh_state_path, state)
            update_batch_state(
                paths.batch_state_path,
                batch_id=normalized_batch,
                last_refresh_ok=True,
            )
            write_latest_tidy_summary(
                paths,
                stage="tidy-refresh",
                status="ok",
                exit_code=0,
                extra={
                    "mode": "incremental",
                    "batch_id": normalized_batch,
                    "touched_files": [str(path) for path in touched_files],
                    "incremental_files": [str(path) for path in incremental_files],
                    "neighbor_scope": effective_neighbor_scope,
                    "pending_tasks": int(manifest.get("task_count", 0)),
                },
            )
            write_tidy_result(
                ctx,
                paths,
                stage="tidy-refresh",
                status="ok",
                exit_code=0,
            )
            return 0

    returncode, _ = run_tidy_build(
        ctx,
        keep_going=effective_keep_going,
        stage="tidy-refresh",
    )
    if returncode != 0:
        update_batch_state(
            paths.batch_state_path,
            batch_id=normalized_batch or None,
            last_refresh_ok=False,
        )
        return returncode

    split_ret, manifest = split_captured_log(
        ctx,
        paths=paths,
        preserve_history=True,
        stage="tidy-refresh",
    )
    if split_ret != 0:
        update_batch_state(
            paths.batch_state_path,
            batch_id=normalized_batch or None,
            last_refresh_ok=False,
        )
        return split_ret

    state["batches_since_full"] = 0
    state["last_full_reason"] = ",".join(full_reasons) or "manual_full"
    state["last_full_batch"] = normalized_batch or state.get("last_full_batch")
    state["last_full_at"] = utc_now_iso()
    update_refresh_state(paths.refresh_state_path, state)
    update_batch_state(
        paths.batch_state_path,
        batch_id=normalized_batch or None,
        last_refresh_ok=True,
    )
    write_latest_tidy_summary(
        paths,
        stage="tidy-refresh",
        status="ok",
        exit_code=0,
        extra={
            "mode": "full",
            "batch_id": normalized_batch,
            "full_reasons": full_reasons,
            "pending_tasks": int(manifest.get("task_count", 0)),
        },
    )
    write_tidy_result(ctx, paths, stage="tidy-refresh", status="ok", exit_code=0)
    return 0


def run(args, ctx: Context) -> int:
    return execute_refresh(
        ctx,
        batch_id=str(args.batch_id or ""),
        full_every=args.full_every,
        force_full=bool(args.force_full),
        final_full=bool(args.final_full),
        dry_run=bool(args.dry_run),
        keep_going=args.keep_going,
        neighbor_scope=args.neighbor_scope,
    )


def _append_unique(target: list[str], values: list[str]) -> None:
    for value in values:
        if value not in target:
            target.append(value)


def _read_text_if_exists(path: Path) -> str:
    if not path.exists():
        return ""
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""
