#!/usr/bin/env python3

from __future__ import annotations

try:
    from ._bootstrap import bootstrap_repo_root
except ImportError:
    from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.verify.checks.check_boundary_layering_support.cli import main

__all__ = ["main"]


if __name__ == "__main__":
    raise SystemExit(main())
