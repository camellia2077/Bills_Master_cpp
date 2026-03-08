from __future__ import annotations

from pathlib import Path


def _relpath(base_dir: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(base_dir.resolve()))
    except ValueError:
        return str(path)


def _norm(text: str) -> str:
    return text.replace("\\", "/").lower()


def _batch_num(batch_id: str) -> int:
    if batch_id.startswith("batch_"):
        try:
            return int(batch_id.replace("batch_", ""))
        except ValueError:
            return 0
    return 0
