from __future__ import annotations

import sys
from pathlib import Path


def bootstrap_test_paths(anchor_file: str) -> tuple[Path, Path]:
    test_suite_dir = Path(anchor_file).resolve().parent
    test_root = test_suite_dir.parents[2]
    repo_root = test_root.parent
    if str(test_root) not in sys.path:
        sys.path.insert(0, str(test_root))
    if str(repo_root) not in sys.path:
        sys.path.insert(0, str(repo_root))
    return test_root, repo_root
