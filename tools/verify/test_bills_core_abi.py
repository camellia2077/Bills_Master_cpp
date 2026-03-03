#!/usr/bin/env python3

from __future__ import annotations

import runpy
from pathlib import Path


def main() -> None:
    repo_root = Path(__file__).resolve().parents[2]
    entry = repo_root / "tests" / "suites" / "logic" / "bills_core_abi" / "run_tests.py"
    runpy.run_path(str(entry), run_name="__main__")


if __name__ == "__main__":
    main()
