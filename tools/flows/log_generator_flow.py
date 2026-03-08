#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from log_generator_build.cli import main as cli_main
from log_generator_build.config_loader import load_config

PROJECT_DIR = REPO_ROOT / "tests" / "generators" / "log_generator"
DEFAULT_CONFIG_PATH = PROJECT_DIR / "scripts" / "config.toml"


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--config", default=str(DEFAULT_CONFIG_PATH))
    args, remaining = parser.parse_known_args(sys.argv[1:])

    config = load_config(Path(args.config).resolve())
    return cli_main(
        config=config,
        project_dir=PROJECT_DIR,
        repo_root=REPO_ROOT,
        argv=remaining,
    )


if __name__ == "__main__":
    raise SystemExit(main())
