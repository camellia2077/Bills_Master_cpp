from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..core.context import Context
from ..core.path_display import display_path
from ..services.clang_tidy_runner import run_clang_tidy
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_residuals import (
    classify_residual_diagnostics,
    load_diagnostics_jsonl,
)
from ..services.tidy_runtime import (
    build_numbering_context,
    resolve_batch_source_files,
    update_batch_phase,
    update_batch_runtime_state,
    write_diagnostics_jsonl,
    write_tidy_result,
)


@dataclass(frozen=True)
class TidyRecheckResult:
    returncode: int
    log_path: Path
    diagnostics_path: Path
    files: list[Path]
    diagnostics: list[dict]
    decision_summary: dict


def run_tidy_recheck_pass(
    ctx: Context,
    *,
    batch_id: str,
) -> TidyRecheckResult:
    paths = resolve_tidy_paths(ctx)
    normalized_batch = batch_id.strip()
    log_path = paths.refresh_dir / f"{normalized_batch}_recheck.log"
    diagnostics_path = paths.refresh_dir / f"{normalized_batch}_recheck.diagnostics.jsonl"
    if not paths.compile_commands_path.exists():
        return TidyRecheckResult(
            returncode=1,
            log_path=log_path,
            diagnostics_path=diagnostics_path,
            files=[],
            diagnostics=[],
            decision_summary={
                "manual_refactor_count": 0,
                "suggest_nolint_count": 0,
                "unexpected_fixable_count": 0,
                "files_with_remaining": [],
            },
        )

    files = resolve_batch_source_files(ctx, paths, batch_id=normalized_batch)
    if not files:
        return TidyRecheckResult(
            returncode=1,
            log_path=log_path,
            diagnostics_path=diagnostics_path,
            files=[],
            diagnostics=[],
            decision_summary={
                "manual_refactor_count": 0,
                "suggest_nolint_count": 0,
                "unexpected_fixable_count": 0,
                "files_with_remaining": [],
            },
        )

    returncode = run_clang_tidy(
        ctx,
        compile_commands_dir=paths.compile_commands_path.parent,
        files=files,
        output_log=log_path,
        fix=False,
    )
    log_content = (
        log_path.read_text(encoding="utf-8", errors="replace") if log_path.exists() else ""
    )
    write_diagnostics_jsonl(log_content=log_content, output_path=diagnostics_path)
    diagnostics = load_diagnostics_jsonl(diagnostics_path)
    classified, decision_summary = classify_residual_diagnostics(
        diagnostics,
        suppression_allowed_patterns=ctx.config.tidy.suppression.allowed_checks,
        safe_fix_patterns=ctx.config.tidy.safe_fix_prepass.checks,
        count_safe_fix_as_unexpected=False,
    )
    return TidyRecheckResult(
        returncode=returncode,
        log_path=log_path,
        diagnostics_path=diagnostics_path,
        files=files,
        diagnostics=classified,
        decision_summary=decision_summary,
    )


def execute_tidy_recheck(ctx: Context, *, batch_id: str) -> int:
    paths = resolve_tidy_paths(ctx)
    normalized_batch = batch_id.strip()
    source_files = resolve_batch_source_files(ctx, paths, batch_id=normalized_batch)
    if not source_files:
        print(f"[ERROR] tidy-recheck could not resolve `{normalized_batch}` source files.")
        write_tidy_result(
            ctx,
            paths,
            stage="tidy-recheck",
            status="failed",
            exit_code=1,
            batch_id=normalized_batch,
            current_phase="recheck",
        )
        return 1

    update_batch_runtime_state(
        paths,
        normalized_batch,
        status="running",
        source_files=[str(path) for path in source_files],
        numbering_context=build_numbering_context(paths, current_batch_id=normalized_batch),
    )
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="recheck",
        status="running",
        details={"files": [str(path) for path in source_files]},
    )
    result = run_tidy_recheck_pass(ctx, batch_id=normalized_batch)
    recheck_status = "completed" if result.returncode == 0 else "failed"
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="recheck",
        status=recheck_status,
        details={
            "log_path": str(result.log_path),
            "diagnostics_path": str(result.diagnostics_path),
            "diagnostics_count": len(result.diagnostics),
        },
    )
    update_batch_phase(
        paths,
        batch_id=normalized_batch,
        phase="classify",
        status="completed" if result.returncode == 0 else "failed",
        details={
            "remaining_count": len(result.diagnostics),
            "decision_summary": result.decision_summary,
        },
    )
    batch_status = "needs_manual" if result.diagnostics else "ready_for_clean_refresh"
    if result.returncode != 0:
        batch_status = "failed"
    update_batch_runtime_state(
        paths,
        normalized_batch,
        status=batch_status,
        current_phase="classify" if result.returncode == 0 else "recheck",
        source_files=[str(path) for path in result.files],
        recheck={
            "status": recheck_status,
            "files": [str(path) for path in result.files],
            "log_path": str(result.log_path),
            "diagnostics_path": str(result.diagnostics_path),
            "returncode": result.returncode,
            "diagnostics_count": len(result.diagnostics),
        },
        remaining={
            "count": len(result.diagnostics),
            "diagnostics": result.diagnostics,
        },
        decision_summary=result.decision_summary,
        numbering_context=build_numbering_context(paths, current_batch_id=normalized_batch),
    )
    write_tidy_result(
        ctx,
        paths,
        stage="tidy-recheck",
        status=batch_status if result.returncode == 0 else "failed",
        exit_code=result.returncode,
        batch_id=normalized_batch,
        current_phase="classify" if result.returncode == 0 else "recheck",
        next_action=(
            None
            if result.returncode == 0
            and int(result.decision_summary.get("unexpected_fixable_count", 0) or 0) > 0
            else (
                f"Next: python tools/run.py tidy-status --batch-id {normalized_batch}"
                if result.returncode == 0
                else None
            )
        ),
    )
    print(f"--- tidy-recheck: log -> {display_path(result.log_path, resolve=True)}")
    print(
        f"--- tidy-recheck: remaining={len(result.diagnostics)} "
        f"manual={result.decision_summary.get('manual_refactor_count', 0)} "
        f"suggest_nolint={result.decision_summary.get('suggest_nolint_count', 0)}"
    )
    return result.returncode


def run(args, ctx: Context) -> int:
    return execute_tidy_recheck(ctx, batch_id=str(args.batch_id))
