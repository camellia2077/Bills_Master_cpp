from __future__ import annotations

import sys

from ..core.context import Context
from .parser import build_parser


def main(argv: list[str] | None = None) -> int:
    effective_argv = list(sys.argv[1:] if argv is None else argv)
    ctx = Context.from_repo()
    parser = build_parser()
    args = parser.parse_args(effective_argv)
    handler = getattr(args, "handler", None)
    if handler is None:
        parser.print_help()
        return 1
    return int(handler(args, ctx))
