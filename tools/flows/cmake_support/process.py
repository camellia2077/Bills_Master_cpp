from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from .models import BuildResult


def run_command(command: list[str], cwd: Path) -> None:
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        raise SystemExit(1)
    except subprocess.CalledProcessError as exc:
        print(f"\n!!! A dist preparation step failed with exit code {exc.returncode}.")
        raise SystemExit(exc.returncode)


def run_build_capture(
    command: list[str],
    *,
    cwd: Path,
    log_path: Path,
    success_message: str = "==> Dist preparation finished successfully.",
    failure_prefix: str = "==> Dist preparation failed with exit code ",
) -> BuildResult:
    print(f"==> Running command: {' '.join(command)}")
    try:
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            cwd=cwd,
        )
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        raise SystemExit(1)

    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_lines: list[str] = []

    assert process.stdout is not None
    with log_path.open("w", encoding="utf-8") as log_file:
        for line in process.stdout:
            log_lines.append(line)
            log_file.write(line)
            print(line, end="")

    process.wait()
    success = process.returncode == 0
    if success:
        print(success_message)
    else:
        print(f"{failure_prefix}{process.returncode}.")
    return BuildResult(log_lines=log_lines, success=success)
