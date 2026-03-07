from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    batch_id = str(args.batch_id).strip()
    tasks = load_batch_tasks(paths.tasks_manifest, batch_id)
    if not tasks:
        print(f"[ERROR] tidy-show could not find `{batch_id}`.")
        return 1

    print(f"{batch_id}: tasks={len(tasks)}")
    for task in tasks:
        print(
            f"- task_{str(task.get('task_id', '')).zfill(3)} "
            f"file={task.get('source_file', '')} "
            f"score={float(task.get('score', 999.0)):.2f} "
            f"strategy={task.get('primary_fix_strategy', '')} "
            f"checks={','.join(task.get('checks', []))}"
        )
    return 0
