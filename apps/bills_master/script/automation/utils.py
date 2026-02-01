import subprocess
import sys
import os
from pathlib import Path
from typing import List

def run_command(command: List[str], cwd: Path):
    """Run a subprocess command in the specified directory with error handling."""
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"\n!!! A build step failed with exit code {e.returncode}.")
        sys.exit(e.returncode)

def setup_environment():
    """Switch to the project root directory and return its path."""
    # script/automation/utils.py -> script/automation -> script -> project_root
    project_dir = Path(__file__).resolve().parent.parent.parent
    os.chdir(project_dir)
    print(f"==> Switched to project directory: {os.getcwd()}")
    return project_dir
