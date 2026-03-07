from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

from ..core.context import Context
from .file_discovery import discover_source_files
from .timestamps import utc_now_iso


def run_format_command(
    ctx: Context,
    *,
    check: bool,
    explicit_paths: list[str],
) -> int:
    clang_format = shutil.which("clang-format")
    if not clang_format:
        print("[ERROR] `clang-format` not found in PATH.")
        return 2

    files = discover_source_files(
        ctx.repo_root,
        scope_config=ctx.config.scope,
        explicit_paths=explicit_paths,
    )
    if not files:
        print("[WARN] No source files matched the format scope.")
        return 0

    run_dir = _make_run_dir(ctx.temp_root / "format" / "runs")
    run_id = run_dir.name
    output_log = run_dir / "output.log"

    changed_files: list[str] = []
    failed_files: list[str] = []
    checked_files: list[str] = []
    started_at = utc_now_iso()

    with output_log.open("w", encoding="utf-8") as handle:
        for file_path in files:
            checked_files.append(_relpath(ctx.repo_root, file_path))
            before_hash = _hash_file(file_path)
            command = [clang_format]
            if check:
                command.extend(["--dry-run", "--Werror"])
            else:
                command.append("-i")
            command.extend(["-style=file", str(file_path)])

            result = ctx.process_runner.run(command, cwd=ctx.repo_root)
            handle.write(f"$ {' '.join(command)}\n")
            handle.write(f"returncode={result.returncode}\n\n")
            if result.returncode != 0:
                failed_files.append(_relpath(ctx.repo_root, file_path))
                if check:
                    changed_files.append(_relpath(ctx.repo_root, file_path))
                continue
            if not check and before_hash != _hash_file(file_path):
                changed_files.append(_relpath(ctx.repo_root, file_path))

    finished_at = utc_now_iso()
    summary = {
        "run_id": run_id,
        "ok": len(failed_files) == 0,
        "status": "ok" if not failed_files else "failed",
        "mode": "check" if check else "apply",
        "checked_files": checked_files,
        "changed_files": changed_files,
        "failed_files": failed_files,
        "started_at": started_at,
        "finished_at": finished_at,
    }
    (run_dir / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(
        "[OK] format completed."
        if not failed_files
        else f"[FAILED] format completed with {len(failed_files)} failing file(s)."
    )
    print(f"--- format summary: {run_dir / 'summary.json'}")
    return 0 if not failed_files else 1


def _make_run_dir(base_dir: Path) -> Path:
    base_dir.mkdir(parents=True, exist_ok=True)
    run_dir = base_dir / utc_now_iso().replace(":", "").replace("-", "")
    run_dir.mkdir(parents=True, exist_ok=True)
    return run_dir


def _hash_file(path: Path) -> str:
    return hashlib.sha1(path.read_bytes()).hexdigest()


def _relpath(repo_root: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(repo_root.resolve()))
    except ValueError:
        return str(path)
