from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks
from ..services.tidy_runtime import (
    load_batch_runtime_state,
    load_latest_state,
    resolve_batch_numbering_context,
    write_tidy_result,
)


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    write_tidy_result(
        ctx,
        paths,
        stage="tidy-status",
        status="ok",
        exit_code=0,
    )
    latest = load_latest_state(paths)

    batch_id = str(getattr(args, "batch_id", "") or "").strip()
    if batch_id:
        return _print_batch_status(paths, batch_id)
    return _print_global_status(latest)


def _print_global_status(latest: dict) -> int:
    queue = latest.get("queue", {})
    numbering = latest.get("numbering_context", {})
    last_run = latest.get("last_run", {})
    last_verify = latest.get("last_verify", {})
    print(
        "tidy-status: "
        f"remaining_tasks={int(queue.get('remaining_tasks', 0) or 0)} "
        f"remaining_batches={int(queue.get('remaining_batches', 0) or 0)}"
    )
    print(f"current_batch={latest.get('current_batch') or '<none>'}")
    print(f"current_phase={latest.get('current_phase') or '<none>'}")
    print(f"next_open_batch={queue.get('next_open_batch') or '<none>'}")
    print(
        "already_closed_before_current="
        f"{int(numbering.get('already_closed_before_current', 0) or 0)}"
    )
    ranges = numbering.get("already_closed_ranges", [])
    print(
        "already_closed_ranges="
        + (",".join(str(item) for item in ranges) if ranges else "<none>")
    )
    print(f"next_action={latest.get('next_action') or '<none>'}")
    print(
        "last_run="
        f"{last_run.get('stage', '<none>')} "
        f"status={last_run.get('status', '<none>')} "
        f"exit_code={last_run.get('exit_code', '<none>')}"
    )
    print(
        "last_verify="
        f"success={bool(last_verify.get('success', False))} "
        f"returncode={last_verify.get('returncode', '<none>')}"
    )
    return 0


def _print_batch_status(paths, batch_id: str) -> int:
    state_path = paths.batch_state_dir / f"{batch_id}.json"
    tasks = load_batch_tasks(paths.tasks_manifest, batch_id)
    if not state_path.exists() and not tasks:
        print(f"[ERROR] tidy-status could not find `{batch_id}`.")
        return 1
    state = load_batch_runtime_state(paths, batch_id)
    numbering = resolve_batch_numbering_context(
        paths,
        batch_id=batch_id,
        batch_state=state,
    )
    source_files = list(state.get("source_files", []))
    if not source_files:
        source_files = sorted(
            {
                str(task.get("source_file", "")).strip()
                for task in tasks
                if str(task.get("source_file", "")).strip()
            }
        )
    print(f"{batch_id}: status={state.get('status', 'open')}")
    print(f"current_phase={state.get('current_phase') or '<none>'}")
    print(f"next_open_batch={numbering.get('next_open_batch') or '<none>'}")
    ranges = numbering.get("already_closed_ranges", [])
    print(
        "already_closed_ranges="
        + (",".join(str(item) for item in ranges) if ranges else "<none>")
    )
    print(f"source_files={len(source_files)}")
    for phase_name, phase_payload in state.get("phases", {}).items():
        print(
            f"- phase={phase_name} status={phase_payload.get('status', 'pending')}"
        )
    remaining = state.get("remaining", {})
    decision = state.get("decision_summary", {})
    print(f"remaining={int(remaining.get('count', 0) or 0)}")
    print(
        "decision_summary="
        f"manual_refactor={int(decision.get('manual_refactor_count', 0) or 0)} "
        f"suggest_nolint={int(decision.get('suggest_nolint_count', 0) or 0)} "
        f"unexpected_fixable={int(decision.get('unexpected_fixable_count', 0) or 0)}"
    )
    diagnostics = list(remaining.get("diagnostics", []))
    for diagnostic in diagnostics[:20]:
        print(
            f"- {diagnostic.get('preferred_action')} "
            f"{diagnostic.get('check')} "
            f"{diagnostic.get('file')}:{diagnostic.get('line')} "
            f"{diagnostic.get('message')}"
        )
    return 0
