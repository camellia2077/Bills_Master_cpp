from __future__ import annotations

from ..core.context import Context
from .tidy_support import run_tidy_build


def run(args, ctx: Context) -> int:
    returncode, _ = run_tidy_build(
        ctx,
        build_dir_name=str(args.build_dir),
        forwarded=list(args.forwarded),
        keep_going=ctx.config.tidy.keep_going,
        stage="tidy",
    )
    return returncode
