from __future__ import annotations

from ..core.context import Context
from ..core.path_display import display_path_from_repo
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks
from ..services.tidy_runtime import (
    load_batch_runtime_state,
    resolve_batch_numbering_context,
)


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    batch_id = str(args.batch_id).strip()
    tasks = load_batch_tasks(paths.tasks_manifest, batch_id)
    state = load_batch_runtime_state(paths, batch_id)
    if not tasks and not state.get("source_files"):
        print(f"[ERROR] tidy-show could not find `{batch_id}`.")
        return 1

    print(
        f"{batch_id}: tasks={len(tasks)} "
        f"status={state.get('status', 'open')} "
        f"current_phase={state.get('current_phase') or '<none>'}"
    )
    numbering = resolve_batch_numbering_context(
        paths,
        batch_id=batch_id,
        batch_state=state,
    )
    ranges = list(numbering.get("already_closed_ranges", []))
    if ranges:
        print("already_closed_ranges=" + ",".join(str(item) for item in ranges))
    remaining = state.get("remaining", {})
    if int(remaining.get("count", 0) or 0) > 0:
        decision = state.get("decision_summary", {})
        print(
            "remaining="
            f"{int(remaining.get('count', 0) or 0)} "
            f"manual_refactor={int(decision.get('manual_refactor_count', 0) or 0)} "
            f"suggest_nolint={int(decision.get('suggest_nolint_count', 0) or 0)}"
        )
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
    phases = state.get("phases", {})
    if isinstance(phases, dict) and phases:
        for phase_name, payload in phases.items():
            print(f"- phase={phase_name} status={payload.get('status', 'pending')}")
    return 0
