#!/usr/bin/env python3

from __future__ import annotations

import argparse
import importlib.util
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    resolve_runtime_project_root,
    resolve_runtime_workspace_dir,
)


def has_dependency(module_name: str) -> bool:
    return importlib.util.find_spec(module_name) is not None


def resolve_default_db(repo_root: Path) -> Path | None:
    runs_root = resolve_runtime_project_root(repo_root, "bills_tracer") / "runs"
    if runs_root.exists():
        candidates = sorted(
            runs_root.glob("*/bills.sqlite3"),
            key=lambda path: path.stat().st_mtime,
            reverse=True,
        )
        if candidates:
            return candidates[0]

    workspace_db = resolve_runtime_workspace_dir(repo_root, "bills_tracer") / "bills.sqlite3"
    if workspace_db.exists():
        return workspace_db
    return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run graph_generator using latest runtime sqlite db if available.",
    )
    parser.add_argument("--type", default="year", choices=["year", "month", "month-details"])
    parser.add_argument("--period", default="2024")
    parser.add_argument("--db", default="", help="Optional explicit sqlite db path.")
    parser.add_argument(
        "--output-dir",
        default="dist/tests/artifact/reporting/graph_generator",
    )
    parser.add_argument("--output-name", default="")
    return parser.parse_args()


def default_output_name(chart_type: str, period: str) -> str:
    if chart_type == "year":
        return f"{period}_yearly_chart.png"
    if chart_type == "month":
        return f"{period}_monthly_chart.png"
    return f"{period}_details"


def main() -> int:
    args = parse_args()
    repo_root = REPO_ROOT

    missing = [
        module_name for module_name in ("pandas", "matplotlib") if not has_dependency(module_name)
    ]
    if missing:
        print(f"[WARN] Skip graph generation, missing dependencies: {missing}")
        return 0

    db_path = Path(args.db).resolve() if args.db.strip() else resolve_default_db(repo_root)
    if db_path is None:
        print("[WARN] Skip graph generation, no sqlite db found in runtime outputs.")
        return 0

    output_dir = Path(args.output_dir)
    if not output_dir.is_absolute():
        output_dir = (repo_root / output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    output_name = args.output_name.strip() or default_output_name(args.type, args.period)
    output_path = output_dir / output_name
    entry = repo_root / "tools" / "reporting" / "graph_generator" / "main.py"
    command = [
        sys.executable,
        str(entry),
        args.type,
        args.period,
        "--db",
        str(db_path),
        "--out",
        str(output_path),
    ]
    print(f"==> Running graph_generator: {' '.join(command)}")
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
