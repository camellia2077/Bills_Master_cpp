from __future__ import annotations

from ..core.context import Context
from ..verify.cli import main as verify_main
from .common import normalize_forwarded_args


def run(args, ctx: Context) -> int:
    forwarded = normalize_forwarded_args(list(args.forwarded))
    return verify_main(
        forwarded,
        repo_root=ctx.repo_root,
        python_exe=ctx.python_executable,
    )
