from __future__ import annotations

import argparse

from ..core.context import Context
from ..services.file_discovery import (
    build_header_filter_regex,
    discover_source_files,
    resolve_header_filter_roots,
)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Export toolchain scope values for CMake integration."
    )
    parser.add_argument(
        "--mode",
        choices=["format-sources", "header-filter"],
        required=True,
    )
    parser.add_argument("--include-optional", action="store_true")
    args = parser.parse_args()

    ctx = Context.from_repo()
    scope_config = ctx.config.scope

    if args.mode == "header-filter":
        value = build_header_filter_regex(resolve_header_filter_roots(scope_config))
    else:
        value = _join_paths(
            discover_source_files(
                ctx.repo_root,
                scope_config=scope_config,
                include_optional_roots=bool(args.include_optional),
            )
        )

    print(value, end="")
    return 0


def _join_paths(paths: list) -> str:
    return ";".join(str(path.resolve()).replace("\\", "/") for path in paths)


if __name__ == "__main__":
    raise SystemExit(main())
