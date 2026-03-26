#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

TOOLS_ROOT = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_ROOT.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.cli.main import main

if __name__ == "__main__":
    raise SystemExit(main())
