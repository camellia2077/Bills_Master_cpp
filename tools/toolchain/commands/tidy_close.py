from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_manifest
from ..services.tidy_runtime import (
    write_latest_tidy_summary,
    write_tidy_result,
    write_verify_result,
)
from ..services.tidy_state import update_batch_state
from .common import run_verify_workflow
from .tidy_refresh import execute_refresh


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    refresh_ret = execute_refresh(
        ctx,
        batch_id="",
        full_every=ctx.config.tidy.full_every,
        final_full=True,
        keep_going=args.keep_going,
    )
    if refresh_ret != 0:
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-close",
            status="failed",
            exit_code=refresh_ret,
        )
        return refresh_ret

    if not args.tidy_only:
        command, verify_ret = run_verify_workflow(
            ctx, "bills-build", ["--preset", "debug", "--scope", "shared"]
        )
        write_verify_result(paths, command=command, returncode=verify_ret)
        if verify_ret != 0:
            write_latest_tidy_summary(
                paths,
                stage="tidy-close",
                status="failed",
                exit_code=verify_ret,
                extra={"failed_stage": "verify"},
            )
            write_tidy_result(
                ctx,
                paths,
                stage="tidy-close",
                status="failed",
                exit_code=verify_ret,
            )
            return verify_ret

    manifest = load_manifest(paths.tasks_manifest)
    pending_logs = list(paths.tasks_dir.rglob("task_*.log"))
    if manifest.get("tasks") or pending_logs:
        print(
            f"[ERROR] tidy-close detected {len(manifest.get('tasks', []))} pending task(s)."
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-close",
            status="failed",
            exit_code=1,
            extra={"reason": "pending_tasks_remain"},
        )
        write_tidy_result(ctx, paths, stage="tidy-close", status="failed", exit_code=1)
        return 1

    update_batch_state(
        paths.batch_state_path,
        batch_id=None,
        last_refresh_ok=True,
        last_verify_success=(None if args.tidy_only else True),
        last_close_ok=True,
    )
    write_latest_tidy_summary(
        paths,
        stage="tidy-close",
        status="ok",
        exit_code=0,
        extra={"tidy_only": bool(args.tidy_only)},
    )
    write_tidy_result(ctx, paths, stage="tidy-close", status="ok", exit_code=0)
    print("--- tidy-close: completed; no pending tidy tasks remain.")
    return 0
