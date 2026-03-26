from __future__ import annotations

from ...services.tidy_runtime import (
    build_numbering_context,
    empty_decision_summary,
    empty_recheck_state,
    empty_remaining_state,
    update_batch_phase,
    update_batch_runtime_state,
    write_tidy_result,
    write_verify_result,
)
from ..clean import execute_clean
from ..common import run_cli_dist_command
from ..tidy_fix import run_tidy_fix_pass
from ..tidy_recheck import run_tidy_recheck_pass
from ..tidy_refresh import execute_refresh
from .helpers import mark_remaining_phases_skipped, print_next_batch_preview, record_terminal_result
from .models import BatchExecutionState, StepOutcome


def run_verify_phase(state: BatchExecutionState) -> StepOutcome:
    if not state.request.run_verify:
        return StepOutcome()
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="verify",
        status="running",
    )
    command, verify_ret = run_cli_dist_command(state.ctx, preset="debug", scope="shared")
    write_verify_result(state.paths, command=command, returncode=verify_ret)
    if verify_ret != 0:
        update_batch_phase(
            state.paths,
            batch_id=state.normalized_batch,
            phase="verify",
            status="failed",
            details={"command": command, "returncode": verify_ret},
        )
        update_batch_runtime_state(
            state.paths,
            state.normalized_batch,
            status="failed",
            current_phase="verify",
            numbering_context=build_numbering_context(
                state.paths,
                current_batch_id=state.normalized_batch,
            ),
        )
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=[
                "safe_fix_prepass",
                "build_gate",
                "recheck",
                "classify",
                "clean_refresh",
                "finalize",
            ],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="failed",
            exit_code=verify_ret,
            current_phase="verify",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "verify",
            },
        )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="verify",
        status="completed",
        details={"command": command, "returncode": verify_ret},
    )
    return StepOutcome()


def run_safe_fix_prepass_phase(state: BatchExecutionState) -> StepOutcome:
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="safe_fix_prepass",
        status="running",
    )
    fix_result = run_tidy_fix_pass(
        state.ctx,
        batch_id=state.normalized_batch,
        explicit_paths=[],
        checks_filter=list(state.ctx.config.tidy.safe_fix_prepass.checks),
    )
    state.fix_result = fix_result
    prepass_status = "completed" if fix_result.returncode == 0 else "failed"
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="safe_fix_prepass",
        status=prepass_status,
        details={
            "log_path": str(fix_result.log_path),
            "changed_files": [str(path) for path in fix_result.changed_files],
            "matched_safe_fix_checks": state.matched_safe_fix_checks,
            "returncode": fix_result.returncode,
        },
    )
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="running" if fix_result.returncode == 0 else "failed",
        current_phase="safe_fix_prepass",
        auto_fix_prepass={
            "status": prepass_status,
            "checks": list(state.ctx.config.tidy.safe_fix_prepass.checks),
            "matched_checks_present": state.matched_safe_fix_checks,
            "target_files": [str(path) for path in fix_result.target_files],
            "changed_files": [str(path) for path in fix_result.changed_files],
            "log_path": str(fix_result.log_path),
            "returncode": fix_result.returncode,
        },
        numbering_context=build_numbering_context(
            state.paths,
            current_batch_id=state.normalized_batch,
        ),
    )
    if fix_result.returncode != 0:
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["build_gate", "recheck", "classify", "clean_refresh", "finalize"],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="failed",
            exit_code=fix_result.returncode,
            current_phase="safe_fix_prepass",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "safe_fix_prepass",
            },
        )
    return StepOutcome()


def run_build_gate_phase(state: BatchExecutionState) -> StepOutcome:
    assert state.fix_result is not None
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="build_gate",
        status="running",
    )
    build_command, build_ret = run_cli_dist_command(
        state.ctx,
        preset="debug",
        scope="shared",
    )
    write_verify_result(state.paths, command=build_command, returncode=build_ret)
    if build_ret != 0:
        suspect_files = [
            str(path) for path in (state.fix_result.changed_files or state.fix_result.target_files)
        ]
        update_batch_phase(
            state.paths,
            batch_id=state.normalized_batch,
            phase="build_gate",
            status="failed",
            details={
                "command": build_command,
                "returncode": build_ret,
                "suspect_files": suspect_files,
            },
        )
        update_batch_runtime_state(
            state.paths,
            state.normalized_batch,
            status="needs_manual_after_fix",
            current_phase="build_gate",
            build_gate={
                "status": "failed",
                "command": build_command,
                "returncode": build_ret,
                "suspect_files": suspect_files,
            },
            recheck=empty_recheck_state(status="skipped"),
            remaining=empty_remaining_state(),
            decision_summary=empty_decision_summary(),
            numbering_context=build_numbering_context(
                state.paths,
                current_batch_id=state.normalized_batch,
            ),
        )
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["recheck", "classify", "clean_refresh", "finalize"],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="needs_manual_after_fix",
            exit_code=build_ret,
            current_phase="build_gate",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "build_gate",
            },
            next_action=f"Next: python tools/run.py tidy-status --batch-id {state.normalized_batch}",
        )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="build_gate",
        status="completed",
        details={"command": build_command, "returncode": build_ret},
    )
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="running",
        current_phase="build_gate",
        build_gate={
            "status": "completed",
            "command": build_command,
            "returncode": build_ret,
            "suspect_files": [],
        },
        numbering_context=build_numbering_context(
            state.paths,
            current_batch_id=state.normalized_batch,
        ),
    )
    return StepOutcome()


def run_recheck_classify_phase(state: BatchExecutionState) -> StepOutcome:
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="recheck",
        status="running",
    )
    recheck_result = run_tidy_recheck_pass(state.ctx, batch_id=state.normalized_batch)
    state.recheck_result = recheck_result
    recheck_status = "completed" if recheck_result.returncode == 0 else "failed"
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="recheck",
        status=recheck_status,
        details={
            "log_path": str(recheck_result.log_path),
            "diagnostics_path": str(recheck_result.diagnostics_path),
            "diagnostics_count": len(recheck_result.diagnostics),
        },
    )
    classify_status = "completed" if recheck_result.returncode == 0 else "failed"
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="classify",
        status=classify_status,
        details={
            "remaining_count": len(recheck_result.diagnostics),
            "decision_summary": recheck_result.decision_summary,
        },
    )
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="running" if recheck_result.returncode == 0 else "failed",
        current_phase="classify" if recheck_result.returncode == 0 else "recheck",
        recheck={
            "status": recheck_status,
            "files": [str(path) for path in recheck_result.files],
            "log_path": str(recheck_result.log_path),
            "diagnostics_path": str(recheck_result.diagnostics_path),
            "returncode": recheck_result.returncode,
            "diagnostics_count": len(recheck_result.diagnostics),
        },
        remaining={
            "count": len(recheck_result.diagnostics),
            "diagnostics": recheck_result.diagnostics,
        },
        decision_summary=recheck_result.decision_summary,
        numbering_context=build_numbering_context(
            state.paths,
            current_batch_id=state.normalized_batch,
        ),
    )
    if recheck_result.returncode != 0:
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["clean_refresh", "finalize"],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="failed",
            exit_code=recheck_result.returncode,
            current_phase="recheck",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "recheck",
            },
        )

    if recheck_result.diagnostics:
        update_batch_runtime_state(
            state.paths,
            state.normalized_batch,
            status="needs_manual",
            current_phase="classify",
            numbering_context=build_numbering_context(
                state.paths,
                current_batch_id=state.normalized_batch,
            ),
        )
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["clean_refresh", "finalize"],
        )
        print(
            f"--- tidy-batch: {state.normalized_batch} requires manual follow-up "
            f"(remaining={len(recheck_result.diagnostics)})"
        )
        return record_terminal_result(
            state,
            summary_status="needs_manual",
            tidy_status="needs_manual",
            exit_code=1,
            current_phase="classify",
            summary_extra={
                "batch_id": state.normalized_batch,
                "remaining_count": len(recheck_result.diagnostics),
                "decision_summary": recheck_result.decision_summary,
            },
            next_action=(
                None
                if int(recheck_result.decision_summary.get("unexpected_fixable_count", 0) or 0)
                > 0
                else f"Next: python tools/run.py tidy-status --batch-id {state.normalized_batch}"
            ),
        )
    return StepOutcome()


def run_clean_refresh_phase(state: BatchExecutionState) -> StepOutcome:
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="clean_refresh",
        status="running",
    )
    clean_ret, clean_result = execute_clean(
        state.ctx,
        batch_id=state.normalized_batch,
        task_ids=[],
        strict=state.request.strict_clean,
        cluster_by_file=True,
        quiet=True,
    )
    if clean_ret != 0 or clean_result is None:
        update_batch_phase(
            state.paths,
            batch_id=state.normalized_batch,
            phase="clean_refresh",
            status="failed",
            details={"clean_returncode": clean_ret or 1},
        )
        update_batch_runtime_state(
            state.paths,
            state.normalized_batch,
            status="failed",
            current_phase="clean_refresh",
            numbering_context=build_numbering_context(
                state.paths,
                current_batch_id=state.normalized_batch,
            ),
        )
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["finalize"],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="failed",
            exit_code=clean_ret or 1,
            current_phase="clean_refresh",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "clean_refresh",
            },
        )

    refresh_ret = execute_refresh(
        state.ctx,
        batch_id=state.normalized_batch,
        full_every=state.effective_full_every,
        keep_going=state.request.keep_going,
    )
    if refresh_ret != 0:
        update_batch_phase(
            state.paths,
            batch_id=state.normalized_batch,
            phase="clean_refresh",
            status="failed",
            details={
                "cleaned_task_ids": clean_result.cleaned_task_ids,
                "refresh_returncode": refresh_ret,
            },
        )
        update_batch_runtime_state(
            state.paths,
            state.normalized_batch,
            status="failed",
            current_phase="clean_refresh",
            numbering_context=build_numbering_context(
                state.paths,
                current_batch_id=state.normalized_batch,
            ),
        )
        mark_remaining_phases_skipped(
            state.paths,
            batch_id=state.normalized_batch,
            phase_names=["finalize"],
        )
        return record_terminal_result(
            state,
            summary_status="failed",
            tidy_status="failed",
            exit_code=refresh_ret,
            current_phase="clean_refresh",
            summary_extra={
                "batch_id": state.normalized_batch,
                "failed_stage": "clean_refresh",
            },
        )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="clean_refresh",
        status="completed",
        details={
            "cleaned_task_ids": clean_result.cleaned_task_ids,
            "refresh_returncode": refresh_ret,
        },
    )
    state.clean_result = clean_result
    return StepOutcome()


def run_finalize_phase(state: BatchExecutionState) -> StepOutcome:
    assert state.clean_result is not None
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="finalize",
        status="running",
    )
    update_batch_runtime_state(
        state.paths,
        state.normalized_batch,
        status="done",
        current_phase="finalize",
        numbering_context=build_numbering_context(
            state.paths,
            current_batch_id=state.normalized_batch,
        ),
    )
    update_batch_phase(
        state.paths,
        batch_id=state.normalized_batch,
        phase="finalize",
        status="completed",
        details={"cleaned_task_ids": state.clean_result.cleaned_task_ids},
    )
    write_tidy_result(
        state.ctx,
        state.paths,
        stage="tidy-batch",
        status="ok",
        exit_code=0,
        batch_id=state.normalized_batch,
        current_phase="finalize",
    )
    from ...services.tidy_runtime import write_latest_tidy_summary

    write_latest_tidy_summary(
        state.paths,
        stage="tidy-batch",
        status="ok",
        exit_code=0,
        extra={
            "batch_id": state.normalized_batch,
            "cleaned_task_ids": state.clean_result.cleaned_task_ids,
            "task_count": len(state.batch_tasks),
        },
    )
    print(
        f"--- tidy-batch: completed {state.normalized_batch} "
        f"(cleaned={len(state.clean_result.cleaned_task_ids)})"
    )
    print_next_batch_preview(state.ctx, state.paths)
    return StepOutcome(stop=True, return_code=0)
