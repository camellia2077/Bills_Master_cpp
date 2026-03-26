from __future__ import annotations

from ..core.context import Context
from ..services.dist.import_gate import run_import_gate


def run(args, ctx: Context) -> int:
    return run_import_gate(
        ctx.repo_root,
        target=str(args.target).strip(),
        preset=str(args.preset).strip(),
        scope=str(args.scope).strip(),
    )
