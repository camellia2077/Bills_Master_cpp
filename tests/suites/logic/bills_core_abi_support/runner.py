from __future__ import annotations

import argparse
from pathlib import Path

from .build import detect_library_path, run_build
from .client import AbiClient
from .scenarios import run_suite


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run C ABI compatibility smoke tests for libs/bills_core."
    )
    parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
        help="Preset used when --skip-dist is not set.",
    )
    parser.add_argument(
        "--lib",
        dest="library_path",
        default="",
        help="Explicit shared library path. If omitted, auto-detect from dist/cmake.",
    )
    parser.add_argument(
        "--skip-dist",
        action="store_true",
        help="Skip invoking tools/flows/build_bills_core.py.",
    )
    parser.add_argument(
        "--smoke-loops",
        type=int,
        default=200,
        help="Loop count for ownership/free smoke test.",
    )
    args = parser.parse_args()

    try:
        if not args.skip_dist:
            run_build(args.preset)

        if args.library_path:
            library_path = Path(args.library_path).resolve()
            if not library_path.is_file():
                raise FileNotFoundError(f"Library not found: {library_path}")
        else:
            library_path = detect_library_path(args.preset)

        print(f"==> Testing ABI library: {library_path}")
        client = AbiClient(library_path)
        run_suite(client, smoke_loops=args.smoke_loops)
    except Exception as exc:  # pylint: disable=broad-except
        print(f"[FAILED] {exc}")
        return 1

    print("[OK] bills_core ABI smoke tests passed.")
    return 0
