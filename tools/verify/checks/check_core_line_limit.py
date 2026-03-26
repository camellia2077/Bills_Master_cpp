#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from pathlib import Path

VALID_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}
IGNORE_DIR_NAMES = {".git", ".idea", ".vscode", "bin", "obj", "debug", "release", "vs"}
IGNORE_DIR_PREFIXES = ("dist", "cmake", "out")


def should_skip_dir(path: Path) -> bool:
    name = path.name.lower()
    return name in IGNORE_DIR_NAMES or any(
        name.startswith(prefix) for prefix in IGNORE_DIR_PREFIXES
    )


def count_file_lines(file_path: Path) -> int:
    with file_path.open("r", encoding="utf-8", errors="ignore") as handle:
        return sum(1 for _ in handle)


def find_files_over_limit(root: Path, limit: int) -> list[tuple[Path, int]]:
    results: list[tuple[Path, int]] = []
    if not root.exists():
        return results

    for current_root, dirs, files in os.walk(root):
        dirs[:] = [name for name in dirs if not should_skip_dir(Path(current_root) / name)]

        for filename in files:
            path = Path(current_root) / filename
            if path.suffix.lower() not in VALID_EXTENSIONS:
                continue

            line_count = count_file_lines(path)
            if line_count > limit:
                results.append((path, line_count))

    results.sort(key=lambda item: item[1], reverse=True)
    return results


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check whether bills_core contains files whose line count exceeds a threshold."
    )
    parser.add_argument(
        "--root",
        default="libs/core/src",
        help="Directory to scan (default: libs/core/src).",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=500,
        help="Line count threshold (default: 500).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()

    if not root.exists():
        print(f"[ERROR] Path does not exist: {root}")
        return 2
    if args.limit < 1:
        print("[ERROR] --limit must be >= 1")
        return 2

    print(f"Scanning: {root}")
    print(f"Line limit: {args.limit}")

    files = find_files_over_limit(root, args.limit)
    if not files:
        print(f"[OK] No files exceed {args.limit} lines.")
        return 0

    print(f"[FOUND] {len(files)} file(s) exceed {args.limit} lines:")
    for file_path, line_count in files:
        print(f"{line_count:>6} | {file_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
