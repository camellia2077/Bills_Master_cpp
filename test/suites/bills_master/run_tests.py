#!/usr/bin/env python3

from __future__ import annotations

import runpy
from pathlib import Path


def main() -> None:
    repo_root = Path(__file__).resolve().parents[3]
    entry = repo_root / "test" / "suites" / "artifact" / "bills_master" / "run_tests.py"
    runpy.run_path(str(entry), run_name="__main__")


if __name__ == "__main__":
    main()
