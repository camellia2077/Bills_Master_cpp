from __future__ import annotations

from pathlib import Path

from tools.flows.log_generator_build.cli import main as cli_main
from tools.flows.log_generator_build.config_loader import load_config

PROJECT_SUBDIR = Path("tests") / "generators" / "log_generator"
CONFIG_SUBPATH = PROJECT_SUBDIR / "scripts" / "config.toml"

PROJECT_DIR = PROJECT_SUBDIR
DEFAULT_CONFIG_PATH = CONFIG_SUBPATH


def run_log_generator_command(repo_root: Path, argv: list[str] | None = None) -> int:
    project_dir = (repo_root / PROJECT_SUBDIR).resolve()
    config_path = (repo_root / CONFIG_SUBPATH).resolve()
    config = load_config(config_path)
    return cli_main(
        config=config,
        project_dir=project_dir,
        repo_root=repo_root.resolve(),
        argv=list(argv or []),
    )
