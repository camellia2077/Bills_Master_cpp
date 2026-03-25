#!/usr/bin/env python3
"""Prepare apps/bills_cli into dist/cmake and then run CLI tests from tests/."""

from __future__ import annotations

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.flows.bills_tracer_flow_support.cmake_dist import build_cli
from tools.flows.bills_tracer_flow_support.config_writer import parse_formats
from tools.flows.bills_tracer_flow_support.runner import main

__all__ = ["build_cli", "main", "parse_formats"]


if __name__ == "__main__":
    raise SystemExit(main())
