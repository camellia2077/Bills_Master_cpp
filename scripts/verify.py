#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def normalize_extra(extra_args: list[str]) -> list[str]:
    if extra_args and extra_args[0] == "--":
        return extra_args[1:]
    return extra_args


def run(command: list[str]) -> int:
    print(f"==> Running: {' '.join(command)}")
    return subprocess.call(command)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Unified verify entry for build and test workflows."
    )
    parser.add_argument(
        "workflow",
        nargs="?",
        default="bills",
        choices=["bills", "bills-build", "core-build", "core-abi", "log-build"],
        help="bills=build+CLI test, bills-build=compile bills_windows_cli, core-build=compile bills_core, core-abi=run ABI smoke tests, log-build=compile log_generator",
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Forwarded arguments for the selected workflow",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    python_exe = sys.executable
    forwarded = normalize_extra(args.extra)

    if args.workflow == "bills":
        entry = repo_root / "scripts" / "build_then_cli_test.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "bills-build":
        entry = repo_root / "scripts" / "build_bills_master.py"
        if not forwarded:
            forwarded = ["build_fast"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-build":
        entry = repo_root / "scripts" / "build_bills_core.py"
        if not forwarded:
            forwarded = ["build_fast", "--shared"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-abi":
        entry = repo_root / "scripts" / "test_bills_core_abi.py"
        return run([python_exe, str(entry), *forwarded])

    entry = repo_root / "scripts" / "build_log_generator.py"
    if not forwarded:
        forwarded = ["build", "--mode", "Debug"]
    return run([python_exe, str(entry), *forwarded])


if __name__ == "__main__":
    raise SystemExit(main())
