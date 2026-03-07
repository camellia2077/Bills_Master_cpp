from __future__ import annotations

import subprocess
import shutil
from pathlib import Path

from ..core.context import Context


def run_clang_tidy(
    ctx: Context,
    *,
    compile_commands_dir: Path,
    files: list[Path],
    output_log: Path,
    fix: bool = False,
) -> int:
    clang_tidy = shutil.which("clang-tidy")
    if not clang_tidy:
        print("[ERROR] `clang-tidy` not found in PATH.")
        return 2
    if not files:
        output_log.parent.mkdir(parents=True, exist_ok=True)
        output_log.write_text("", encoding="utf-8")
        return 0

    output_log.parent.mkdir(parents=True, exist_ok=True)
    returncode = 0
    with output_log.open("w", encoding="utf-8") as handle:
        total = len(files)
        for index, file_path in enumerate(files, start=1):
            command = [clang_tidy]
            if fix:
                command.append("-fix")
            command.extend(
                ["-p", str(compile_commands_dir.resolve()), str(file_path.resolve())]
            )
            header = f"[{index}/{total}] Analyzing: {file_path.resolve()}\n"
            handle.write(header)
            print(header, end="")
            print(f"==> Running: {' '.join(command)}")
            process = subprocess.Popen(
                command,
                cwd=ctx.repo_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            assert process.stdout is not None
            for line in process.stdout:
                handle.write(line)
                print(line, end="")
            file_returncode = int(process.wait())
            handle.write(f"[returncode] {file_returncode}\n\n")
            if file_returncode != 0 and returncode == 0:
                returncode = file_returncode
    return returncode
