from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from ..core.context import Context
from ..core.path_display import display_command, display_path


def normalize_checks_filter(checks_filter: list[str] | None) -> list[str]:
    if not checks_filter:
        return []
    normalized: list[str] = []
    for pattern in checks_filter:
        text = str(pattern).strip()
        if not text or text == "-*":
            continue
        if text not in normalized:
            normalized.append(text)
    return ["-*", *normalized]


def run_clang_tidy(
    ctx: Context,
    *,
    compile_commands_dir: Path,
    files: list[Path],
    output_log: Path,
    fix: bool = False,
    checks_filter: list[str] | None = None,
    extra_args: list[str] | None = None,
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
    effective_checks_filter = normalize_checks_filter(checks_filter)
    with output_log.open("w", encoding="utf-8") as handle:
        total = len(files)
        for index, file_path in enumerate(files, start=1):
            command = [clang_tidy]
            if fix:
                command.append("-fix")
            if effective_checks_filter:
                command.append(f"-checks={','.join(effective_checks_filter)}")
            if extra_args:
                command.extend(extra_args)
            command.extend(["-p", str(compile_commands_dir.resolve()), str(file_path.resolve())])
            header = f"[{index}/{total}] Analyzing: {display_path(file_path, resolve=True)}\n"
            handle.write(header)
            print(header, end="")
            print(f"==> Running: {display_command(command)}")
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
