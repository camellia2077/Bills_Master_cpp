from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import next_open_batch


def run(args, ctx: Context) -> int:
    del args
    paths = resolve_tidy_paths(ctx)
    batch_id = next_open_batch(paths.tasks_manifest)
    if not batch_id:
        print("--- tidy-next: no pending tidy batch.")
        return 0
    print(batch_id)
    print(f"Next: python tools/run.py tidy-batch --batch-id {batch_id} --preset sop")
    return 0
