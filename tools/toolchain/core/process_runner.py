from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path

from .path_display import display_command


@dataclass(frozen=True)
class ProcessResult:
    command: list[str]
    cwd: Path
    returncode: int


class ProcessRunner:
    def run(
        self,
        command: list[str],
        *,
        cwd: Path,
        check: bool = False,
    ) -> ProcessResult:
        print(f"==> Running: {display_command(command)}")
        completed = subprocess.run(command, cwd=cwd, check=False)
        result = ProcessResult(
            command=list(command),
            cwd=cwd,
            returncode=int(completed.returncode),
        )
        if check and result.returncode != 0:
            raise subprocess.CalledProcessError(result.returncode, command)
        return result
