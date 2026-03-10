from __future__ import annotations

from ..core.context import Context
from ..services.tidy_paths import resolve_tidy_paths
from .tidy_support import split_captured_log


def run(args, ctx: Context) -> int:
    paths = resolve_tidy_paths(ctx)
    returncode, _ = split_captured_log(
        ctx,
        paths=paths,
        max_lines=args.max_lines,
        max_diags=args.max_diags,
        batch_size=args.batch_size,
        preserve_history=True,
        stage="tidy-split",
    )
    return returncode
