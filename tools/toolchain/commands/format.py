from __future__ import annotations

from ..core.context import Context
from ..services.format_runner import run_format_command


def run(args, ctx: Context) -> int:
    return run_format_command(
        ctx,
        check=bool(args.check),
        explicit_paths=list(args.paths),
    )
