from __future__ import annotations

from pathlib import Path

DEFAULT_SOURCE_ROOTS = [
    "apps/bills_cli/src",
    "libs/bills_core/src",
    "libs/bills_io/src",
    "tests/generators/log_generator/src",
]

SOURCE_EXTENSIONS = {".cpp", ".cc", ".cxx", ".hpp", ".h", ".hxx"}
EXCLUDED_SEGMENTS = {".git", "build", "build_fast", "build_tidy", "build_debug", "temp"}


def discover_source_files(
    repo_root: Path,
    *,
    roots: list[str] | None = None,
    explicit_paths: list[str] | None = None,
) -> list[Path]:
    files: dict[str, Path] = {}
    if explicit_paths:
        for raw_path in explicit_paths:
            candidate = (repo_root / raw_path).resolve() if not Path(raw_path).is_absolute() else Path(raw_path).resolve()
            if candidate.is_file() and candidate.suffix.lower() in SOURCE_EXTENSIONS:
                files[_path_key(candidate)] = candidate
            elif candidate.is_dir():
                for item in _walk_source_files(candidate):
                    files[_path_key(item)] = item
        return sorted(files.values(), key=lambda path: _path_key(path))

    selected_roots = roots if roots is not None else DEFAULT_SOURCE_ROOTS
    for root in selected_roots:
        base_dir = repo_root / root
        if not base_dir.exists():
            continue
        for item in _walk_source_files(base_dir):
            files[_path_key(item)] = item
    return sorted(files.values(), key=lambda path: _path_key(path))


def _walk_source_files(base_dir: Path) -> list[Path]:
    results: list[Path] = []
    for path in base_dir.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in SOURCE_EXTENSIONS:
            continue
        if any(segment in EXCLUDED_SEGMENTS for segment in path.parts):
            continue
        results.append(path.resolve())
    return results


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()
