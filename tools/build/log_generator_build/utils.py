import os
import subprocess
import sys
from pathlib import Path
from typing import List


def run_command(command: List[str], cwd: Path) -> None:
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(
            f"!!! Error: Command '{command[0]}' not found. "
            "Is it installed and in your PATH?"
        )
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        print(f"\n!!! A build step failed with exit code {exc.returncode}.")
        sys.exit(exc.returncode)


def setup_environment(project_dir: Path) -> Path:
    os.chdir(project_dir)
    print(f"==> Switched to project directory: {os.getcwd()}")
    return project_dir
