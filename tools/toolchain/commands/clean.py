from __future__ import annotations

from ..core.context import Context
from ..services.tidy_cleaner import CleanResult, clean_tasks
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_runtime import (
    latest_verify_succeeded,
    write_latest_tidy_summary,
    write_tidy_result,
)
from ..services.tidy_state import set_batch_status, update_batch_state


def execute_clean(
    ctx: Context,
    *,
    batch_id: str | None,
    task_ids: list[str],
    strict: bool,
    cluster_by_file: bool,
    quiet: bool = False,
) -> tuple[int, CleanResult | None]:
    paths = resolve_tidy_paths(ctx)
    normalized_batch = (batch_id or "").strip() or None
    try:
        result = clean_tasks(
            ctx,
            paths,
            batch_id=normalized_batch,
            task_ids=task_ids,
            strict=strict,
            cluster_by_file=cluster_by_file,
        )
    except ValueError as exc:
        print(f"[ERROR] {exc}")
        write_latest_tidy_summary(
            paths,
            stage="clean",
            status="failed",
            exit_code=1,
            extra={"reason": str(exc), "batch_id": normalized_batch},
        )
        write_tidy_result(ctx, paths, stage="clean", status="failed", exit_code=1)
        return 1, None

    if result.archived_count <= 0:
        print("[WARN] clean did not archive any task logs.")
        write_latest_tidy_summary(
            paths,
            stage="clean",
            status="failed",
            exit_code=1,
            extra={"reason": "no_tasks_archived", "batch_id": normalized_batch},
        )
        write_tidy_result(ctx, paths, stage="clean", status="failed", exit_code=1)
        return 1, result

    verify_success = latest_verify_succeeded(paths)[0] if strict else None
    update_batch_state(
        paths.batch_state_path,
        batch_id=normalized_batch,
        cleaned_task_ids=result.cleaned_task_ids,
        last_verify_success=verify_success,
        last_clean_ok=True,
    )
    if normalized_batch:
        set_batch_status(
            paths.batch_status_path,
            normalized_batch,
            "cleaned",
            cleaned_task_ids=result.cleaned_task_ids,
            touched_files=[str(path) for path in result.touched_files],
        )

    write_latest_tidy_summary(
        paths,
        stage="clean",
        status="ok",
        exit_code=0,
        extra={
            "batch_id": normalized_batch,
            "archived_count": result.archived_count,
            "cleaned_task_ids": result.cleaned_task_ids,
            "strict": strict,
            "cluster_by_file": cluster_by_file,
        },
    )
    write_tidy_result(ctx, paths, stage="clean", status="ok", exit_code=0)
    if not quiet:
        print(
            "--- clean: archived "
            f"{result.archived_count} task log(s)"
            + (f" from {normalized_batch}" if normalized_batch else "")
        )
    return 0, result


def run(args, ctx: Context) -> int:
    if not args.batch_id and not args.task_ids:
        print("[ERROR] clean requires --batch-id or explicit task ids.")
        return 2
    returncode, _ = execute_clean(
        ctx,
        batch_id=args.batch_id,
        task_ids=list(args.task_ids),
        strict=bool(args.strict),
        cluster_by_file=bool(args.cluster_by_file),
    )
    return returncode
