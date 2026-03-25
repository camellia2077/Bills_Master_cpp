import os
from pathlib import Path

from tools.flows.cmake_support.process import run_command


def setup_environment(project_dir: Path) -> Path:
    os.chdir(project_dir)
    print(f"==> Switched to project directory: {os.getcwd()}")
    return project_dir
