from __future__ import annotations

from time import monotonic

from ..core.context import Context
from ..core.path_display import display_path_from_repo
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks, next_open_batch
from ..services.tidy_runtime import (
    build_numbering_context,
    empty_build_gate_state,
    empty_decision_summary,
    empty_recheck_state,
    empty_remaining_state,
    load_latest_state,
    resolve_batch_source_files,
    update_batch_phase,
    update_batch_runtime_state,
    write_latest_tidy_summary,
    write_tidy_result,
    write_verify_result,
)
from .clean import execute_clean
from .common import run_verify_workflow
from .tidy_fix import run_tidy_fix_pass
from .tidy_recheck import run_tidy_recheck_pass
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

    started = monotonic()
    effective_full_every = (
        ctx.config.tidy.full_every if full_every is None else int(full_every)
    )
    source_files = resolve_batch_source_files(ctx, paths, batch_id=normalized_batch)
    matched_safe_fix_checks = sorted(
        {
            str(check).strip()
            for task in batch_tasks
            for check in task.get("safe_fix_checks_present", [])
            if str(check).strip()
        }
    )
    numbering_context = build_numbering_context(
        paths,
        current_batch_id=normalized_batch,
    )
    update_batch_runtime_state(
        paths,
        normalized_batch,
        status="running",
        current_phase="load_queue",
        source_files=[str(path) for path in source_files],
        build_gate=empty_build_gate_state(),
        recheck=empty_recheck_state(),
        remaining=empty_remaining_state(),
        decision_summary=empty_decision_summary(),
        numbering_context=numbering_context,
    )
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="load_queue",
        status="completed",
        details={
            "task_count": len(batch_tasks),
            "source_file_count": len(source_files),
            "matched_safe_fix_checks": matched_safe_fix_checks,
        },
    )
    write_tidy_result(
        ctx,
        paths,
        stage="tidy-batch",
        status="running",
        exit_code=0,
        batch_id=normalized_batch,
        current_phase="load_queue",
    )

    if run_verify:
        timeout_ret = _check_timeout(
            started, timeout_seconds, ctx, paths, normalized_batch, "verify"
        )
        if timeout_ret is not None:
            return timeout_ret
        update_batch_phase(
            paths,
            batch_id=normalized_batch,
            phase="verify",
            status="running",
        )
        command, verify_ret = run_verify_workflow(
            ctx, "bills-build", ["--preset", "debug", "--scope", "shared"]
        )
        write_verify_result(paths, command=command, returncode=verify_ret)
        if verify_ret != 0:
            update_batch_phase(
                paths,
                batch_id=normalized_batch,
                phase="verify",
                status="failed",
                details={"command": command, "returncode": verify_ret},
            )
            update_batch_runtime_state(
                paths,
                normalized_batch,
                status="failed",
                current_phase="verify",
                numbering_context=build_numbering_context(
                    paths, current_batch_id=normalized_batch
                ),
            )
            _mark_remaining_phases_skipped(
                paths,
                batch_id=normalized_batch,
                phase_names=[
                    "safe_fix_prepass",
                    "build_gate",
                    "recheck",
                    "classify",
                    "clean_refresh",
                    "finalize",
                ],
            )
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
                batch_id=normalized_batch,
                current_phase="verify",
            )
            return verify_ret
        update_batch_phase(
            paths,
            batch_id=normalized_batch,
            phase="verify",
            status="completed",
            details={"command": command, "returncode": verify_ret},
        )

    timeout_ret = _check_timeout(
        started, timeout_seconds, ctx, paths, normalized_batch, "safe_fix_prepass"
    )
    if timeout_ret is not None:
        return timeout_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="safe_fix_prepass",
        status="running",
    )
    fix_result = run_tidy_fix_pass(
        ctx,
        batch_id=normalized_batch,
        explicit_paths=[],
        checks_filter=list(ctx.config.tidy.safe_fix_prepass.checks),
    )
    prepass_status = "completed" if fix_result.returncode == 0 else "failed"
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="safe_fix_prepass",
        status=prepass_status,
        details={
            "log_path": str(fix_result.log_path),
            "changed_files": [str(path) for path in fix_result.changed_files],
            "matched_safe_fix_checks": matched_safe_fix_checks,
            "returncode": fix_result.returncode,
        },
    )
    update_batch_runtime_state(
        paths,
        normalized_batch,
        status="running" if fix_result.returncode == 0 else "failed",
        current_phase="safe_fix_prepass",
        auto_fix_prepass={
            "status": prepass_status,
            "checks": list(ctx.config.tidy.safe_fix_prepass.checks),
            "matched_checks_present": matched_safe_fix_checks,
            "target_files": [str(path) for path in fix_result.target_files],
            "changed_files": [str(path) for path in fix_result.changed_files],
            "log_path": str(fix_result.log_path),
            "returncode": fix_result.returncode,
        },
        numbering_context=build_numbering_context(
            paths, current_batch_id=normalized_batch
        ),
    )
    if fix_result.returncode != 0:
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["build_gate", "recheck", "classify", "clean_refresh", "finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=fix_result.returncode,
            extra={"batch_id": normalized_batch, "failed_stage": "safe_fix_prepass"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=fix_result.returncode,
            batch_id=normalized_batch,
            current_phase="safe_fix_prepass",
        )
        return fix_result.returncode

    timeout_ret = _check_timeout(
        started, timeout_seconds, ctx, paths, normalized_batch, "build_gate"
    )
    if timeout_ret is not None:
        return timeout_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="build_gate",
        status="running",
    )
    build_command, build_ret = run_verify_workflow(
        ctx, "bills-build", ["--preset", "debug", "--scope", "shared"]
    )
    write_verify_result(paths, command=build_command, returncode=build_ret)
    if build_ret != 0:
        suspect_files = [
            str(path)
            for path in (fix_result.changed_files or fix_result.target_files)
        ]
        update_batch_phase(
            paths,
            batch_id=normalized_batch,
            phase="build_gate",
            status="failed",
            details={
                "command": build_command,
                "returncode": build_ret,
                "suspect_files": suspect_files,
            },
        )
        update_batch_runtime_state(
            paths,
            normalized_batch,
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
                paths, current_batch_id=normalized_batch
            ),
        )
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["recheck", "classify", "clean_refresh", "finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=build_ret,
            extra={"batch_id": normalized_batch, "failed_stage": "build_gate"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="needs_manual_after_fix",
            exit_code=build_ret,
            batch_id=normalized_batch,
            current_phase="build_gate",
            next_action=f"Next: python tools/run.py tidy-status --batch-id {normalized_batch}",
        )
        return build_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="build_gate",
        status="completed",
        details={"command": build_command, "returncode": build_ret},
    )
    update_batch_runtime_state(
        paths,
        normalized_batch,
        status="running",
        current_phase="build_gate",
        build_gate={
            "status": "completed",
            "command": build_command,
            "returncode": build_ret,
            "suspect_files": [],
        },
        numbering_context=build_numbering_context(
            paths, current_batch_id=normalized_batch
        ),
    )

    timeout_ret = _check_timeout(
        started, timeout_seconds, ctx, paths, normalized_batch, "recheck"
    )
    if timeout_ret is not None:
        return timeout_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="recheck",
        status="running",
    )
    recheck_result = run_tidy_recheck_pass(ctx, batch_id=normalized_batch)
    recheck_status = "completed" if recheck_result.returncode == 0 else "failed"
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
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
        paths,
        batch_id=normalized_batch,
        phase="classify",
        status=classify_status,
        details={
            "remaining_count": len(recheck_result.diagnostics),
            "decision_summary": recheck_result.decision_summary,
        },
    )
    update_batch_runtime_state(
        paths,
        normalized_batch,
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
            paths, current_batch_id=normalized_batch
        ),
    )
    if recheck_result.returncode != 0:
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["clean_refresh", "finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=recheck_result.returncode,
            extra={"batch_id": normalized_batch, "failed_stage": "recheck"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=recheck_result.returncode,
            batch_id=normalized_batch,
            current_phase="recheck",
        )
        return recheck_result.returncode

    if recheck_result.diagnostics:
        update_batch_runtime_state(
            paths,
            normalized_batch,
            status="needs_manual",
            current_phase="classify",
            numbering_context=build_numbering_context(
                paths, current_batch_id=normalized_batch
            ),
        )
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["clean_refresh", "finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="needs_manual",
            exit_code=1,
            extra={
                "batch_id": normalized_batch,
                "remaining_count": len(recheck_result.diagnostics),
                "decision_summary": recheck_result.decision_summary,
            },
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="needs_manual",
            exit_code=1,
            batch_id=normalized_batch,
            current_phase="classify",
            next_action=(
                None
                if int(
                    recheck_result.decision_summary.get("unexpected_fixable_count", 0)
                    or 0
                )
                > 0
                else f"Next: python tools/run.py tidy-status --batch-id {normalized_batch}"
            ),
        )
        print(
            f"--- tidy-batch: {normalized_batch} requires manual follow-up "
            f"(remaining={len(recheck_result.diagnostics)})"
        )
        return 1

    timeout_ret = _check_timeout(
        started, timeout_seconds, ctx, paths, normalized_batch, "clean_refresh"
    )
    if timeout_ret is not None:
        return timeout_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="clean_refresh",
        status="running",
    )
    clean_ret, clean_result = execute_clean(
        ctx,
        batch_id=normalized_batch,
        task_ids=[],
        strict=strict_clean,
        cluster_by_file=True,
        quiet=True,
    )
    if clean_ret != 0 or clean_result is None:
        update_batch_phase(
            paths,
            batch_id=normalized_batch,
            phase="clean_refresh",
            status="failed",
            details={"clean_returncode": clean_ret or 1},
        )
        update_batch_runtime_state(
            paths,
            normalized_batch,
            status="failed",
            current_phase="clean_refresh",
            numbering_context=build_numbering_context(
                paths, current_batch_id=normalized_batch
            ),
        )
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=clean_ret or 1,
            extra={"batch_id": normalized_batch, "failed_stage": "clean_refresh"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=clean_ret or 1,
            batch_id=normalized_batch,
            current_phase="clean_refresh",
        )
        return clean_ret or 1

    refresh_ret = execute_refresh(
        ctx,
        batch_id=normalized_batch,
        full_every=effective_full_every,
        keep_going=keep_going,
    )
    if refresh_ret != 0:
        update_batch_phase(
            paths,
            batch_id=normalized_batch,
            phase="clean_refresh",
            status="failed",
            details={
                "cleaned_task_ids": clean_result.cleaned_task_ids,
                "refresh_returncode": refresh_ret,
            },
        )
        update_batch_runtime_state(
            paths,
            normalized_batch,
            status="failed",
            current_phase="clean_refresh",
            numbering_context=build_numbering_context(
                paths, current_batch_id=normalized_batch
            ),
        )
        _mark_remaining_phases_skipped(
            paths,
            batch_id=normalized_batch,
            phase_names=["finalize"],
        )
        write_latest_tidy_summary(
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=refresh_ret,
            extra={"batch_id": normalized_batch, "failed_stage": "clean_refresh"},
        )
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-batch",
            status="failed",
            exit_code=refresh_ret,
            batch_id=normalized_batch,
            current_phase="clean_refresh",
        )
        return refresh_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="clean_refresh",
        status="completed",
        details={
            "cleaned_task_ids": clean_result.cleaned_task_ids,
            "refresh_returncode": refresh_ret,
        },
    )

    timeout_ret = _check_timeout(
        started, timeout_seconds, ctx, paths, normalized_batch, "finalize"
    )
    if timeout_ret is not None:
        return timeout_ret
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="finalize",
        status="running",
    )
    update_batch_runtime_state(
        paths,
        normalized_batch,
        status="done",
        current_phase="finalize",
        numbering_context=build_numbering_context(
            paths, current_batch_id=normalized_batch
        ),
    )
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="finalize",
        status="completed",
        details={"cleaned_task_ids": clean_result.cleaned_task_ids},
    )
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
    write_tidy_result(
        ctx,
        paths,
        stage="tidy-batch",
        status="ok",
        exit_code=0,
        batch_id=normalized_batch,
        current_phase="finalize",
    )
    print(
        f"--- tidy-batch: completed {normalized_batch} "
        f"(cleaned={len(clean_result.cleaned_task_ids)})"
    )
    _print_next_batch_preview(ctx, paths)
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


def _check_timeout(
    started: float,
    timeout_seconds: int | None,
    ctx: Context,
    paths,
    batch_id: str,
    next_stage: str,
) -> int | None:
    if timeout_seconds is None or timeout_seconds <= 0:
        return None
    if monotonic() - started <= timeout_seconds:
        return None
    print(f"[ERROR] tidy-batch soft timeout reached before stage `{next_stage}`.")
    update_batch_runtime_state(
        paths,
        batch_id,
        status="timeout",
        current_phase=next_stage,
        numbering_context=build_numbering_context(paths, current_batch_id=batch_id),
    )
    update_batch_phase(
        paths,
        batch_id=batch_id,
        phase=next_stage,
        status="failed",
        details={"reason": "timeout"},
    )
    write_tidy_result(
        ctx,
        paths,
        stage="tidy-batch",
        status="timeout",
        exit_code=1,
        batch_id=batch_id,
        current_phase=next_stage,
    )
    return 1


def _mark_remaining_phases_skipped(
    paths,
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


def _print_next_batch_preview(ctx: Context, paths) -> None:
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
