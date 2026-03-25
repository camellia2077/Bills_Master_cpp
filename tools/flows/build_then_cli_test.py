#!/usr/bin/env python3
"""Deprecated compatibility wrapper for bills tracer flow."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

DEPRECATION_START_DATE = "2026-03-05"
SUNSET_DATE = "2026-06-30"


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    flow_entry = repo_root / "tools" / "flows" / "bills_tracer_flow.py"
    forwarded_args = sys.argv[1:]
    print(
        "[DEPRECATED] `tools/flows/build_then_cli_test.py` is a compatibility "
        f"entry (deprecated since {DEPRECATION_START_DATE}) and is planned for "
        f"removal after {SUNSET_DATE}. "
        "Use `python tools/verify/verify.py bills-tracer -- ...` or "
        "`python tools/flows/bills_tracer_flow.py ...`.",
        file=sys.stderr,
    )
    command = [sys.executable, str(flow_entry), *forwarded_args]
    print(f"==> Forwarding to flow: {' '.join(command)}")
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
