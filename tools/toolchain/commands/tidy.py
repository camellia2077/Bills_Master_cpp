from __future__ import annotations

from ..core.context import Context
from .tidy_support import run_tidy_build


def run(args, ctx: Context) -> int:
    returncode, _ = run_tidy_build(
        ctx,
        forwarded=list(args.forwarded),
        jobs=args.jobs,
        keep_going=args.keep_going,
        stage="tidy",
    )
    return returncode
