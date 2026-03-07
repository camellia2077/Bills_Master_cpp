from __future__ import annotations

import json
import shutil
from pathlib import Path

from ..core.context import Context
from .tidy_paths import TidyPaths
from .tidy_queue import load_manifest
from .timestamps import utc_now_iso


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


def write_verify_result(paths: TidyPaths, *, command: list[str], returncode: int) -> None:
    payload = {
        "generated_at": utc_now_iso(),
        "success": returncode == 0,
        "returncode": int(returncode),
        "command": command,
    }
    paths.verify_result_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def latest_verify_succeeded(paths: TidyPaths) -> tuple[bool, str]:
    if not paths.verify_result_path.exists():
        return False, "missing verify result"
    try:
        payload = json.loads(paths.verify_result_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False, "invalid verify result"
    if not isinstance(payload, dict):
        return False, "invalid verify result"
    if bool(payload.get("success")):
        return True, "ok"
    return False, "latest verify returned failure"


def latest_verify_result_mtime(paths: TidyPaths) -> float | None:
    if not paths.verify_result_path.exists():
        return None
    try:
        return paths.verify_result_path.stat().st_mtime
    except OSError:
        return None


def write_tidy_result(
    ctx: Context,
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    next_action: str | None = None,
) -> None:
    manifest = load_manifest(paths.tasks_manifest)
    pending_tasks = manifest.get("tasks", [])
    batch_ids = sorted({str(task.get("batch_id")) for task in pending_tasks})
    blocking_files = [
        {
            "task_id": task.get("task_id"),
            "batch_id": task.get("batch_id"),
            "source_file": task.get("source_file"),
            "checks": task.get("checks", []),
            "primary_fix_strategy": task.get("primary_fix_strategy"),
        }
        for task in pending_tasks[:20]
    ]
    if next_action is None:
        if batch_ids:
            first_task = pending_tasks[0]
            next_action = (
                "Next: python tools/run.py tidy-batch --batch-id "
                f"{first_task.get('batch_id')} --preset sop"
            )
        else:
            next_action = "No pending tidy tasks."
    payload = {
        "generated_at": utc_now_iso(),
        "app": "bills",
        "stage": stage,
        "status": status,
        "exit_code": int(exit_code),
        "tasks": {
            "remaining": len(pending_tasks),
            "batches": len(batch_ids),
        },
        "blocking_files": blocking_files,
        "next_action": next_action,
        "config_snapshot": {
            "max_lines": ctx.config.tidy.max_lines,
            "max_diags": ctx.config.tidy.max_diags,
            "batch_size": ctx.config.tidy.batch_size,
        },
    }
    paths.tidy_result_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def extract_task_source_files(task_path: Path) -> list[Path]:
    files: list[Path] = []
    try:
        lines = task_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return files
    for line in lines:
        if not line.startswith("File: "):
            continue
        raw = line[len("File: ") :].strip()
        if not raw:
            continue
        path = Path(raw)
        if path not in files:
            files.append(path)
    return files


def load_pending_tasks_with_content(tasks_root: Path, manifest_path: Path) -> list[dict]:
    manifest = load_manifest(manifest_path)
    tasks: list[dict] = []
    for item in manifest.get("tasks", []):
        content_path = tasks_root.parent / str(item.get("content_path", ""))
        if not content_path.exists():
            continue
        try:
            content = content_path.read_text(encoding="utf-8")
        except OSError:
            continue
        tasks.append(
            {
                "task_id": str(item.get("task_id", "")),
                "batch_id": str(item.get("batch_id", "")),
                "content": content,
                "size": int(item.get("size", len(content))),
                "diagnostics": [{}] * int(item.get("diagnostic_count", 0)),
                "source_file": str(item.get("source_file", "")),
                "checks": list(item.get("checks", [])),
                "primary_fix_strategy": str(item.get("primary_fix_strategy", "manual_only")),
                "score": float(item.get("score", 999.0)),
            }
        )
    return tasks
