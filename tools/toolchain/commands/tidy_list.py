from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_manifest
from ..services.tidy_state import load_batch_status


def run(args, ctx: Context) -> int:
    del args
    paths = resolve_tidy_paths(ctx)
    manifest = load_manifest(paths.tasks_manifest)
    batch_status = load_batch_status(paths.batch_status_path).get("batches", {})
    batches = manifest.get("batches", [])
    if not batches:
        print("--- tidy-list: no pending tidy batches.")
        return 0

    for batch in batches:
        batch_id = str(batch.get("batch_id", ""))
        status = str(batch_status.get(batch_id, {}).get("status", "open"))
        print(
            f"{batch_id}: status={status} "
            f"tasks={int(batch.get('task_count', 0))} "
            f"strategies={','.join(batch.get('primary_fix_strategy', []))}"
        )
    return 0
