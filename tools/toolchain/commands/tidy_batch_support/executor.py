from __future__ import annotations

from time import monotonic

from ...core.context import Context
from ...services.tidy_paths import resolve_tidy_paths
from ...services.tidy_queue import load_batch_tasks
from ...services.tidy_runtime import (
    build_numbering_context,
    empty_build_gate_state,
    empty_decision_summary,
    empty_recheck_state,
    empty_remaining_state,
    resolve_batch_source_files,
    update_batch_phase,
    update_batch_runtime_state,
    write_tidy_result,
)
from .helpers import check_timeout
from .models import BatchExecutionRequest, BatchExecutionState
from .steps import (
    run_build_gate_phase,
    run_clean_refresh_phase,
    run_finalize_phase,
    run_recheck_classify_phase,
    run_safe_fix_prepass_phase,
    run_verify_phase,
)


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
    request = BatchExecutionRequest(
        batch_id=batch_id,
        strict_clean=strict_clean,
        run_verify=run_verify,
        full_every=full_every,
        keep_going=keep_going,
        concise=concise,
        timeout_seconds=timeout_seconds,
    )
    del concise

    paths = resolve_tidy_paths(ctx)
    normalized_batch = request.batch_id.strip()
    batch_tasks = load_batch_tasks(paths.tasks_manifest, normalized_batch)
    if not batch_tasks:
        print(f"[ERROR] tidy-batch could not find pending batch `{normalized_batch}`.")
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=1,
            batch_id=normalized_batch,
            current_phase="load_queue",
        )
        return 1

    effective_full_every = ctx.config.tidy.full_every if full_every is None else int(full_every)
    source_files = resolve_batch_source_files(ctx, paths, batch_id=normalized_batch)
    matched_safe_fix_checks = sorted(
        {
            str(check).strip()
            for task in batch_tasks
            for check in task.get("safe_fix_checks_present", [])
            if str(check).strip()
        }
    )
    state = BatchExecutionState(
        ctx=ctx,
        paths=paths,
        request=request,
        started=monotonic(),
        normalized_batch=normalized_batch,
        batch_tasks=batch_tasks,
        effective_full_every=effective_full_every,
        source_files=source_files,
        matched_safe_fix_checks=matched_safe_fix_checks,
    )
    _initialize_batch_state(state)

    phases = [
        ("verify", run_verify_phase, request.run_verify),
        ("safe_fix_prepass", run_safe_fix_prepass_phase, True),
        ("build_gate", run_build_gate_phase, True),
        ("recheck", run_recheck_classify_phase, True),
        ("clean_refresh", run_clean_refresh_phase, True),
        ("finalize", run_finalize_phase, True),
    ]
    for phase_name, runner, enabled in phases:
        if not enabled:
            continue
        timeout_outcome = check_timeout(state, phase_name)
        if timeout_outcome is not None:
            return timeout_outcome.return_code
        outcome = runner(state)
        if outcome.stop:
            return outcome.return_code
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


def _initialize_batch_state(state: BatchExecutionState) -> None:
    numbering_context = build_numbering_context(
        state.paths,
        current_batch_id=state.normalized_batch,
    )
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="running",
        current_phase="load_queue",
        source_files=[str(path) for path in state.source_files],
        build_gate=empty_build_gate_state(),
        recheck=empty_recheck_state(),
        remaining=empty_remaining_state(),
        decision_summary=empty_decision_summary(),
        numbering_context=numbering_context,
    )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="load_queue",
        status="completed",
        details={
            "task_count": len(state.batch_tasks),
            "source_file_count": len(state.source_files),
            "matched_safe_fix_checks": state.matched_safe_fix_checks,
        },
    )
    write_tidy_result(
        state.ctx,
        state.paths,
        stage="tidy-batch",
        status="running",
        exit_code=0,
        batch_id=state.normalized_batch,
        current_phase="load_queue",
    )
