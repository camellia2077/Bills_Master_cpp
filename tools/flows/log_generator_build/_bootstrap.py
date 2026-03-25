from __future__ import annotations

import sys
from pathlib import Path


def bootstrap_repo_root(anchor_file: str, *, levels_up: int = 3) -> Path:
    repo_root = Path(anchor_file).resolve().parents[levels_up]
    if str(repo_root) not in sys.path:
        sys.path.insert(0, str(repo_root))
    return repo_root
