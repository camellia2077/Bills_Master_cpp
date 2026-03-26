from __future__ import annotations

import argparse
from pathlib import Path

from .common import normalize_extra
from .registry import workflow_help_text, workflow_registry, workflow_specs


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Unified verify entry for test workflows."
    )
    parser.add_argument(
        "workflow",
        nargs="?",
        default="bills-tracer",
        choices=[spec.name for spec in workflow_specs()],
        help=workflow_help_text(),
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Forwarded arguments for the selected workflow",
    )
    return parser


def dispatch_workflow(
    workflow: str,
    *,
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    registry = workflow_registry()
    spec = registry.get(workflow)
    if spec is None:
        raise AssertionError(f"Unhandled workflow: {workflow}")
    return spec.handler(repo_root, python_exe, forwarded)


def main(
    argv: list[str] | None = None,
    *,
    repo_root: Path,
    python_exe: str,
) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    forwarded = normalize_extra(args.extra)
    return dispatch_workflow(
        args.workflow,
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
    )
