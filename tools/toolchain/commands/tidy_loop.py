from __future__ import annotations

import json

from ..core.context import Context
from ..services.fix_strategy import STRATEGY_AUTO_FIX, STRATEGY_NOLINT_ALLOWED
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks, next_open_batch
from ..services.timestamps import utc_now_iso
from .tidy_batch import execute_tidy_batch
from .tidy_fix import execute_tidy_fix


AUTO_LOOP_STRATEGIES = {STRATEGY_AUTO_FIX, STRATEGY_NOLINT_ALLOWED}


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    loop_dir = paths.root / "loops" / utc_now_iso().replace(":", "").replace("-", "")
    loop_dir.mkdir(parents=True, exist_ok=True)

    target_count = -1 if args.all else max(int(args.n or 1), 1)
    processed_batches: list[str] = []
    status = "completed"
    reason = "no_pending_batches"

    while target_count < 0 or len(processed_batches) < target_count:
        batch_id = next_open_batch(paths.tasks_manifest)
        if not batch_id:
            status = "completed"
            reason = "no_pending_batches"
            break

        batch_tasks = load_batch_tasks(paths.tasks_manifest, batch_id)
        strategies = {
            str(task.get("primary_fix_strategy", "")).strip()
            for task in batch_tasks
            if str(task.get("primary_fix_strategy", "")).strip()
        }
        if not strategies or not strategies.issubset(AUTO_LOOP_STRATEGIES):
            status = "stopped_manual"
            reason = f"{batch_id} contains non-auto-fix strategies"
            print(f"--- tidy-loop: stopping at {batch_id} ({reason}).")
            break

        fix_ret = execute_tidy_fix(
            ctx,
            batch_id=batch_id,
            explicit_paths=[],
            output_log=loop_dir / f"{batch_id}_fix.log",
        )
        if fix_ret != 0:
            status = "fix_failed"
            reason = batch_id
            break

        batch_ret = execute_tidy_batch(
            ctx,
            batch_id=batch_id,
            strict_clean=True,
            run_verify=True,
            full_every=ctx.config.tidy.full_every,
            keep_going=True,
            concise=bool(args.concise),
            timeout_seconds=None,
        )
        if batch_ret != 0:
            status = "batch_failed"
            reason = batch_id
            break
        processed_batches.append(batch_id)

    summary = {
        "generated_at": utc_now_iso(),
        "status": status,
        "reason": reason,
        "processed_batches": processed_batches,
        "requested_count": ("all" if args.all else target_count),
        "test_every": int(args.test_every),
        "note": "Loop currently enforces verify-per-batch to keep strict clean safe.",
    }
    (loop_dir / "loop_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(f"--- tidy-loop: summary -> {loop_dir / 'loop_summary.json'}")
    if status in {"completed", "stopped_manual"}:
        return 0
    return 1
