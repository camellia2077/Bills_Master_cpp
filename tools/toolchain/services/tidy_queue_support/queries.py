from __future__ import annotations

from pathlib import Path

from .manifest_store import load_manifest
from .utils import _batch_num


def load_batch_tasks(tasks_manifest: Path, batch_id: str) -> list[dict]:
    manifest = load_manifest(tasks_manifest)
    return [task for task in manifest.get("tasks", []) if str(task.get("batch_id")) == batch_id]


def next_open_batch(tasks_manifest: Path) -> str | None:
    manifest = load_manifest(tasks_manifest)
    batches = sorted(
        {
            str(task.get("batch_id"))
            for task in manifest.get("tasks", [])
            if str(task.get("batch_id", "")).startswith("batch_")
        }
    )
    return batches[0] if batches else None


def max_indices(tasks_manifest: Path, tasks_done_dir: Path) -> tuple[int, int]:
    task_ids: list[int] = []
    batch_ids: list[int] = []
    manifest = load_manifest(tasks_manifest)
    for task in manifest.get("tasks", []):
        task_ids.append(int(str(task.get("task_id", "0"))))
        batch_ids.append(_batch_num(str(task.get("batch_id", ""))))
    for task_path in tasks_done_dir.rglob("task_*.log"):
        try:
            task_ids.append(int(task_path.stem.replace("task_", "")))
        except ValueError:
            pass
        batch_ids.append(_batch_num(task_path.parent.name))
    return (max(task_ids) if task_ids else 0, max(batch_ids) if batch_ids else 0)
