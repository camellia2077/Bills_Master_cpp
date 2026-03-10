from __future__ import annotations

import argparse
import sys

from ..core.context import Context
from .parser import build_parser


def parse_cli_args(
    argv: list[str] | None = None,
) -> tuple[argparse.ArgumentParser, argparse.Namespace]:
    effective_argv = list(sys.argv[1:] if argv is None else argv)
    parser = build_parser()
    args, unknown = parser.parse_known_args(effective_argv)
    if unknown:
        if hasattr(args, "forwarded"):
            existing = list(getattr(args, "forwarded", []))
            args.forwarded = existing + unknown
        else:
            parser.error(f"unrecognized arguments: {' '.join(unknown)}")
    return parser, args


def main(argv: list[str] | None = None) -> int:
    ctx = Context.from_repo()
    parser, args = parse_cli_args(argv)
    handler = getattr(args, "handler", None)
    if handler is None:
        parser.print_help()
        return 1
    return int(handler(args, ctx))
