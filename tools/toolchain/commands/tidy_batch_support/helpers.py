from __future__ import annotations

from ...core.context import Context
from ...core.path_display import display_path_from_repo
from ...services.tidy_paths import TidyPaths
from ...services.tidy_queue import load_batch_tasks, next_open_batch
from ...services.tidy_runtime import (
    build_numbering_context,
    load_latest_state,
    update_batch_phase,
    update_batch_runtime_state,
    write_latest_tidy_summary,
    write_tidy_result,
)
from .models import BatchExecutionState, StepOutcome


def check_timeout(state: BatchExecutionState, next_stage: str) -> StepOutcome | None:
    timeout_seconds = state.request.timeout_seconds
    if timeout_seconds is None or timeout_seconds <= 0:
        return None
    from time import monotonic

    if monotonic() - state.started <= timeout_seconds:
        return None
    print(f"[ERROR] tidy-batch soft timeout reached before stage `{next_stage}`.")
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="timeout",
        current_phase=next_stage,
        numbering_context=build_numbering_context(
            state.paths,
            current_batch_id=state.normalized_batch,
        ),
    )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase=next_stage,
        status="failed",
        details={"reason": "timeout"},
    )
    write_tidy_result(
        state.ctx,
        state.paths,
        stage="tidy-batch",
        status="timeout",
        exit_code=1,
        batch_id=state.normalized_batch,
        current_phase=next_stage,
    )
    return StepOutcome(stop=True, return_code=1)


def mark_remaining_phases_skipped(
    paths: TidyPaths,
    *,
    batch_id: str,
    phase_names: list[str],
) -> None:
    for phase_name in phase_names:
        update_batch_phase(
            paths,
            batch_id=batch_id,
            phase=phase_name,
            status="skipped",
            advance_current_phase=False,
        )


def record_terminal_result(
    state: BatchExecutionState,
    *,
    summary_status: str,
    tidy_status: str,
    exit_code: int,
    current_phase: str,
    summary_extra: dict | None = None,
    next_action: str | None = None,
) -> StepOutcome:
    write_latest_tidy_summary(
        state.paths,
        stage="tidy-batch",
        status=summary_status,
        exit_code=exit_code,
        extra=summary_extra,
    )
    write_tidy_result(
        state.ctx,
        state.paths,
        stage="tidy-batch",
        status=tidy_status,
        exit_code=exit_code,
        batch_id=state.normalized_batch,
        current_phase=current_phase,
        next_action=next_action,
    )
    return StepOutcome(stop=True, return_code=exit_code)


def print_next_batch_preview(ctx: Context, paths: TidyPaths) -> None:
    latest = load_latest_state(paths)
    queue = latest.get("queue", {})
    next_batch_id = str(queue.get("next_open_batch", "")).strip() or next_open_batch(
        paths.tasks_manifest
    )
    if not next_batch_id:
        print("--- tidy-next: no pending tidy batch.")
        return

    print(next_batch_id)
    print(f"Next: python tools/run.py tidy-batch --batch-id {next_batch_id} --preset sop")
    numbering = build_numbering_context(paths, current_batch_id=next_batch_id)
    ranges = list(numbering.get("already_closed_ranges", []))
    if ctx.config.tidy.status.explain_closed_ranges and ranges:
        print("Already closed before current: " + ", ".join(str(item) for item in ranges))

    tasks = load_batch_tasks(paths.tasks_manifest, next_batch_id)
    if not tasks:
        print(f"[ERROR] tidy-show could not find `{next_batch_id}`.")
        return

    print(f"{next_batch_id}: tasks={len(tasks)}")
    for task in tasks:
        source_file = display_path_from_repo(
            ctx.repo_root,
            str(task.get("source_file", "")),
        )
        print(
            f"- task_{str(task.get('task_id', '')).zfill(3)} "
            f"file={source_file} "
            f"score={float(task.get('score', 999.0)):.2f} "
            f"strategy={task.get('primary_fix_strategy', '')} "
            f"checks={','.join(task.get('checks', []))}"
        )
