#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path


def main() -> int:
    repo_root = Path(__file__).resolve().parents[4]
    entry = repo_root / "tools" / "run.py"
    command = [sys.executable, str(entry), "log-generator", *sys.argv[1:]]
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
