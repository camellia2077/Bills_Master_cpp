from __future__ import annotations

import json
import shutil
from pathlib import Path

from ..tidy_log_parser import extract_diagnostics, group_sections
from ..tidy_paths import TidyPaths
from ..timestamps import utc_now_iso


def write_latest_tidy_summary(
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    extra: dict | None = None,
) -> None:
    payload = {
        "generated_at": utc_now_iso(),
        "stage": stage,
        "status": status,
        "exit_code": int(exit_code),
    }
    if extra:
        payload.update(extra)
    paths.latest_summary.parent.mkdir(parents=True, exist_ok=True)
    paths.latest_summary.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def copy_build_log_to_raw(paths: TidyPaths) -> bool:
    if not paths.build_log_path.exists():
        return False
    paths.latest_build_log.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.build_log_path, paths.latest_build_log)
    return True


def new_tidy_run_dir(paths: TidyPaths) -> Path:
    run_dir = paths.run_root / utc_now_iso().replace(":", "").replace("-", "")
    (run_dir / "raw").mkdir(parents=True, exist_ok=True)
    return run_dir


def copy_build_log_to_run(paths: TidyPaths, run_dir: Path) -> Path | None:
    if not paths.build_log_path.exists():
        return None
    destination = run_dir / "raw" / "build.log"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.build_log_path, destination)
    return destination


def write_diagnostics_jsonl(
    *,
    log_content: str,
    output_path: Path,
) -> int:
    diagnostics: list[dict] = []
    for section in group_sections(log_content.splitlines()):
        diagnostics.extend(extract_diagnostics(section))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as handle:
        for item in diagnostics:
            handle.write(json.dumps(item, ensure_ascii=False) + "\n")
    return len(diagnostics)


def copy_compile_commands_to_run(paths: TidyPaths, run_dir: Path) -> Path | None:
    if not paths.compile_commands_path.exists():
        return None
    destination = run_dir / "raw" / "compile_commands.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.compile_commands_path, destination)
    return destination
