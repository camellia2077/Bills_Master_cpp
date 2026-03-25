#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.flows.bills_tracer_flow_support.config_distribution import (  # noqa: E402
    CONFIG_FILENAMES,
    distribute_configs,
    normalize_targets,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Distribute canonical config TOML files into per-platform directories.",
    )
    parser.add_argument(
        "--source-root",
        default=str(REPO_ROOT / "config"),
        help="Source config directory. Defaults to repo-root/config.",
    )
    parser.add_argument(
        "--output-root",
        default=str(REPO_ROOT / "dist" / "config"),
        help="Output root directory. Defaults to repo-root/dist/config.",
    )
    parser.add_argument(
        "--targets",
        default="windows,android",
        help="Comma-separated targets to generate. Defaults to windows,android.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    source_root = Path(args.source_root).resolve()
    output_root = Path(args.output_root).resolve()

    try:
        targets = normalize_targets(str(args.targets))
        outputs = distribute_configs(source_root, output_root, targets)
    except (FileNotFoundError, ValueError, OSError) as error:
        print(f"Config distribution failed: {error}")
        return 2

    for target in targets:
        target_dir = outputs[target]
        print(f"[OK] {target}: {target_dir}")
        for config_name in CONFIG_FILENAMES:
            print(f"  - {target_dir / config_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
