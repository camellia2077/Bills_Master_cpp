from __future__ import annotations

from pathlib import Path

from .common import parse_forwarded_args
from .pipeline_helpers import run_pipeline_workflow


def run_reporting_tools(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    def configure_parser(parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--skip-compile2pdf", action="store_true")
        parser.add_argument("--skip-graph", action="store_true")

    args, passthrough = parse_forwarded_args(forwarded, configure_parser)

    if passthrough:
        print(f"[WARN] reporting-tools ignores extra args: {' '.join(passthrough)}")

    if not args.skip_compile2pdf:
        code = run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
            default_config="tools/verify/pipelines/reporting_compile2pdf.toml",
        )
        if code != 0:
            return code

    if not args.skip_graph:
        code = run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
            default_config="tools/verify/pipelines/reporting_graph_generator.toml",
        )
        if code != 0:
            return code

    return 0
