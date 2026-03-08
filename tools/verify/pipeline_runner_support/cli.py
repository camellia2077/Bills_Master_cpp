from __future__ import annotations

import argparse

from .executor import run_pipeline


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generic TOML-based dist/test pipeline runner.",
    )
    parser.add_argument(
        "--config",
        required=True,
        help="TOML pipeline config path.",
    )
    parser.add_argument(
        "--run-id",
        default="",
        help="Optional custom run id.",
    )
    parser.add_argument(
        "--list-steps",
        action="store_true",
        help="List steps in topological order and exit.",
    )
    return parser.parse_args()


def main() -> int:
    return run_pipeline(parse_args())
