from __future__ import annotations

from time import monotonic

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks
from ..services.tidy_runtime import (
    write_latest_tidy_summary,
    write_tidy_result,
    write_verify_result,
)
from ..services.tidy_state import set_batch_status, update_batch_state, write_json
from .clean import execute_clean
from .common import run_verify_workflow
from .tidy_refresh import execute_refresh


def execute_tidy_batch(
    ctx: Context,
    *,
    batch_id: str,
    strict_clean: bool = False,
    run_verify: bool = False,
    full_every: int | None = None,
    keep_going: bool | None = None,
    concise: bool = False,
    timeout_seconds: int | None = None,
) -> int:
    del concise
    paths = resolve_tidy_paths(ctx)
    normalized_batch = batch_id.strip()
    batch_tasks = load_batch_tasks(paths.tasks_manifest, normalized_batch)
    if not batch_tasks:
        print(f"[ERROR] tidy-batch could not find pending batch `{normalized_batch}`.")
        set_batch_status(paths.batch_status_path, normalized_batch, "missing")
        write_tidy_result(ctx, paths, stage="tidy-batch", status="failed", exit_code=1)
        return 1

    started = monotonic()
    effective_full_every = (
        ctx.config.tidy.full_every if full_every is None else int(full_every)
    )
    set_batch_status(
        paths.batch_status_path,
        normalized_batch,
        "running",
        task_count=len(batch_tasks),
        stage="verify",
    )

    verify_success: bool | None = None
    if run_verify:
        _write_checkpoint(paths, normalized_batch, "verify", "running")
        command, verify_ret = run_verify_workflow(ctx, "bills-build", ["build_fast"])
        write_verify_result(paths, command=command, returncode=verify_ret)
        verify_success = verify_ret == 0
        update_batch_state(
            paths.batch_state_path,
            batch_id=normalized_batch,
            last_verify_success=verify_success,
        )
        if verify_ret != 0:
            set_batch_status(
                paths.batch_status_path,
                normalized_batch,
                "failed",
                stage="verify",
            )
            _write_checkpoint(paths, normalized_batch, "verify", "failed")
            write_latest_tidy_summary(
                paths,
                stage="tidy-batch",
                status="failed",
                exit_code=verify_ret,
                extra={"batch_id": normalized_batch, "failed_stage": "verify"},
            )
            write_tidy_result(
                ctx,
                paths,
                stage="tidy-batch",
                status="failed",
                exit_code=verify_ret,
            )
            return verify_ret

    timeout_ret = _check_timeout(started, timeout_seconds, paths, normalized_batch, "clean")
    if timeout_ret is not None:
        return timeout_ret

    _write_checkpoint(paths, normalized_batch, "clean", "running")
    clean_ret, clean_result = execute_clean(
        ctx,
        batch_id=normalized_batch,
        task_ids=[],
        strict=strict_clean,
        cluster_by_file=True,
        quiet=True,
    )
    if clean_ret != 0 or clean_result is None:
        set_batch_status(
            paths.batch_status_path,
            normalized_batch,
            "failed",
            stage="clean",
        )
        _write_checkpoint(paths, normalized_batch, "clean", "failed")
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=clean_ret or 1,
            extra={"batch_id": normalized_batch, "failed_stage": "clean"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=clean_ret or 1,
        )
        return clean_ret or 1

    update_batch_state(
        paths.batch_state_path,
        batch_id=normalized_batch,
        cleaned_task_ids=clean_result.cleaned_task_ids,
        last_verify_success=verify_success,
        last_clean_ok=True,
    )
    set_batch_status(
        paths.batch_status_path,
        normalized_batch,
        "cleaned",
        stage="refresh",
        cleaned_task_ids=clean_result.cleaned_task_ids,
    )

    timeout_ret = _check_timeout(started, timeout_seconds, paths, normalized_batch, "refresh")
    if timeout_ret is not None:
        return timeout_ret

    _write_checkpoint(paths, normalized_batch, "refresh", "running")
    refresh_ret = execute_refresh(
        ctx,
        batch_id=normalized_batch,
        full_every=effective_full_every,
        keep_going=keep_going,
    )
    if refresh_ret != 0:
        set_batch_status(
            paths.batch_status_path,
            normalized_batch,
            "failed",
            stage="refresh",
        )
        _write_checkpoint(paths, normalized_batch, "refresh", "failed")
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=refresh_ret,
            extra={"batch_id": normalized_batch, "failed_stage": "refresh"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=refresh_ret,
        )
        return refresh_ret

    _write_checkpoint(paths, normalized_batch, "finalize", "running")
    set_batch_status(
        paths.batch_status_path,
        normalized_batch,
        "done",
        stage="done",
        cleaned_task_ids=clean_result.cleaned_task_ids,
    )
    update_batch_state(
        paths.batch_state_path,
        batch_id=normalized_batch,
        cleaned_task_ids=clean_result.cleaned_task_ids,
        last_refresh_ok=True,
    )
    _write_checkpoint(paths, normalized_batch, "done", "completed")
    write_latest_tidy_summary(
        paths,
        stage="tidy-batch",
        status="ok",
        exit_code=0,
        extra={
            "batch_id": normalized_batch,
            "cleaned_task_ids": clean_result.cleaned_task_ids,
            "task_count": len(batch_tasks),
        },
    )
    write_tidy_result(ctx, paths, stage="tidy-batch", status="ok", exit_code=0)
    print(
        f"--- tidy-batch: completed {normalized_batch} "
        f"(cleaned={len(clean_result.cleaned_task_ids)})"
    )
    return 0


def run(args, ctx: Context) -> int:
    if args.preset == "sop":
        args.strict_clean = True
        args.run_verify = True
        args.concise = True
        if args.keep_going is None:
            args.keep_going = True
        if args.full_every is None:
            args.full_every = 3

    return execute_tidy_batch(
        ctx,
        batch_id=str(args.batch_id),
        strict_clean=bool(args.strict_clean),
        run_verify=bool(args.run_verify),
        full_every=args.full_every,
        keep_going=args.keep_going,
        concise=bool(args.concise),
        timeout_seconds=args.timeout_seconds,
    )


def _write_checkpoint(paths, batch_id: str, stage: str, status: str) -> None:
    write_json(
        paths.checkpoint_path,
        {
            "batch_id": batch_id,
            "stage": stage,
            "status": status,
        },
    )


def _check_timeout(
    started: float,
    timeout_seconds: int | None,
    paths,
    batch_id: str,
    next_stage: str,
) -> int | None:
    if timeout_seconds is None or timeout_seconds <= 0:
        return None
    if monotonic() - started <= timeout_seconds:
        return None
    _write_checkpoint(paths, batch_id, next_stage, "timeout")
    print(f"[ERROR] tidy-batch soft timeout reached before stage `{next_stage}`.")
    return 1
