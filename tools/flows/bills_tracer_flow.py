#!/usr/bin/env python3
"""Prepare apps/bills_cli into dist/cmake and then run CLI tests from tests/."""

from __future__ import annotations

import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.flows.bills_tracer_flow_support.cmake_dist import build_cli
from tools.flows.bills_tracer_flow_support.config_writer import parse_formats
from tools.flows.bills_tracer_flow_support.runner import main

__all__ = ["build_cli", "main", "parse_formats"]


if __name__ == "__main__":
    raise SystemExit(main())
