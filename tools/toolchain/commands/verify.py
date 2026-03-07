from __future__ import annotations

from ..core.context import Context
from .common import forward_python_entry


def run(args, ctx: Context) -> int:
    return forward_python_entry(
        ctx,
        ctx.verify_entry(),
        forwarded=args.forwarded,
        default_args=[],
    )
