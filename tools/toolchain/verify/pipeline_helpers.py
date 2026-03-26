from __future__ import annotations

import argparse
import json
import tempfile
from datetime import datetime
from pathlib import Path

from ..services.build_layout import sanitize_segment

from .common import parse_forwarded_args, run


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(json.dumps(item) for item in items) + "]"


def render_pipeline_config(
    *,
    pipeline_name: str,
    pipeline_description: str,
    output_root: str,
    steps: list[dict],
    default_timeout_seconds: int = 7200,
) -> str:
    lines = [
        "schema_version = 1",
        "",
        "[pipeline]",
        f"name = {json.dumps(pipeline_name)}",
        f"description = {json.dumps(pipeline_description)}",
        f"default_timeout_seconds = {default_timeout_seconds}",
        "",
        "[output]",
        f"root = {json.dumps(output_root)}",
        "",
        "[env]",
        'PYTHONUNBUFFERED = "1"',
        "",
    ]
    for step in steps:
        lines.extend(
            [
                "[[steps]]",
                f"id = {json.dumps(step['id'])}",
                f"name = {json.dumps(step.get('name', step['id']))}",
                f"command = {to_toml_list(step['command'])}",
                f"cwd = {json.dumps(step.get('cwd', '{repo_root}'))}",
                f"timeout_seconds = {int(step.get('timeout_seconds', default_timeout_seconds))}",
                f"retries = {int(step.get('retries', 0))}",
                f"depends_on = {to_toml_list([str(item) for item in step.get('depends_on', [])])}",
                f"artifacts = {to_toml_list([str(item) for item in step.get('artifacts', [])])}",
                "",
            ]
        )
    return "\n".join(lines)


def run_pipeline_steps(
    *,
    repo_root: Path,
    python_exe: str,
    pipeline_name: str,
    pipeline_description: str,
    output_root: str,
    steps: list[dict],
    run_id_prefix: str = "",
) -> int:
    config_text = render_pipeline_config(
        pipeline_name=pipeline_name,
        pipeline_description=pipeline_description,
        output_root=output_root,
        steps=steps,
    )
    temp_dir = repo_root / "temp"
    temp_dir.mkdir(parents=True, exist_ok=True)
    run_id = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    runner_entry = repo_root / "tools" / "verify" / "pipeline_runner.py"

    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".toml",
        prefix=f"verify_pipeline_{sanitize_segment(pipeline_name)}_",
        dir=temp_dir,
        delete=False,
        encoding="utf-8",
    ) as handle:
        config_path = Path(handle.name)
        handle.write(config_text)

    try:
        command = [python_exe, str(runner_entry), "--config", str(config_path)]
        if run_id_prefix:
            command.extend(["--run-id", f"{sanitize_segment(run_id_prefix)}_{run_id}"])
        return run(command)
    finally:
        config_path.unlink(missing_ok=True)


def run_pipeline_workflow(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
    default_config: str = "",
) -> int:
    def configure_parser(parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--config", default=default_config)
        parser.add_argument("--run-id", default="")
        parser.add_argument("--list-steps", action="store_true")

    args, passthrough = parse_forwarded_args(forwarded, configure_parser)

    config_path = args.config.strip()
    if not config_path:
        print("[ERROR] pipeline-run requires --config <path>.")
        return 2

    runner_entry = repo_root / "tools" / "verify" / "pipeline_runner.py"
    command = [python_exe, str(runner_entry), "--config", config_path]
    if args.run_id.strip():
        command.extend(["--run-id", args.run_id.strip()])
    if args.list_steps:
        command.append("--list-steps")
    command.extend(passthrough)
    return run(command)
