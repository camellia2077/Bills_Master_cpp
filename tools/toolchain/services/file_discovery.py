from __future__ import annotations

import re
from pathlib import Path

from ..core.config import ScopeConfig

DEFAULT_SOURCE_ROOTS = [
    "apps/bills_cli/src",
    "libs/bills_core/src",
    "libs/bills_io/src",
]
OPTIONAL_SOURCE_ROOTS = ["tests/generators/log_generator/src"]
DEFAULT_HEADER_FILTER_ROOTS = [
    "apps/bills_cli/src",
    "libs/bills_core/src",
    "libs/bills_io/src",
]

SOURCE_EXTENSIONS = {".cpp", ".cc", ".cxx", ".hpp", ".h", ".hxx"}
COMPILE_UNIT_EXTENSIONS = {".cpp", ".cc", ".cxx"}
EXCLUDED_SEGMENTS = {
    ".git",
    "build",
    "build_fast",
    "build_tidy",
    "build_debug",
    "temp",
}


def discover_source_files(
    repo_root: Path,
    *,
    scope_config: ScopeConfig | None = None,
    roots: list[str] | None = None,
    explicit_paths: list[str] | None = None,
    extensions: set[str] | None = None,
    include_optional_roots: bool = False,
) -> list[Path]:
    allowed_extensions = _normalize_extensions(extensions or SOURCE_EXTENSIONS)
    excluded_segments = resolve_excluded_segments(scope_config)
    files: dict[str, Path] = {}
    if explicit_paths:
        for raw_path in explicit_paths:
            candidate = (
                (repo_root / raw_path).resolve()
                if not Path(raw_path).is_absolute()
                else Path(raw_path).resolve()
            )
            if candidate.is_file() and candidate.suffix.lower() in allowed_extensions:
                files[_path_key(candidate)] = candidate
            elif candidate.is_dir():
                for item in _walk_source_files(
                    candidate,
                    extensions=allowed_extensions,
                    excluded_segments=excluded_segments,
                ):
                    files[_path_key(item)] = item
        return sorted(files.values(), key=lambda path: _path_key(path))

    selected_roots = resolve_source_roots(
        scope_config,
        roots=roots,
        include_optional_roots=include_optional_roots,
    )
    for root in selected_roots:
        base_dir = repo_root / root
        if not base_dir.exists():
            continue
        for item in _walk_source_files(
            base_dir,
            extensions=allowed_extensions,
            excluded_segments=excluded_segments,
        ):
            files[_path_key(item)] = item
    return sorted(files.values(), key=lambda path: _path_key(path))


def resolve_source_roots(
    scope_config: ScopeConfig | None = None,
    *,
    roots: list[str] | None = None,
    include_optional_roots: bool = False,
) -> list[str]:
    if roots is not None:
        return _unique_strings(roots)

    configured_roots = (
        list(scope_config.default_roots)
        if scope_config is not None
        else list(DEFAULT_SOURCE_ROOTS)
    )
    if include_optional_roots:
        configured_roots.extend(
            scope_config.optional_roots
            if scope_config is not None
            else OPTIONAL_SOURCE_ROOTS
        )
    return _unique_strings(configured_roots)


def resolve_header_filter_roots(scope_config: ScopeConfig | None = None) -> list[str]:
    roots = (
        list(scope_config.header_filter_roots)
        if scope_config is not None
        else list(DEFAULT_HEADER_FILTER_ROOTS)
    )
    return _unique_strings(roots)


def resolve_excluded_segments(scope_config: ScopeConfig | None = None) -> set[str]:
    if scope_config is None:
        return set(EXCLUDED_SEGMENTS)
    return {segment.strip() for segment in scope_config.exclude_segments if segment.strip()}


def build_header_filter_regex(roots: list[str]) -> str:
    normalized_roots = _unique_strings(roots)
    if not normalized_roots:
        return ".*"

    patterns: list[str] = []
    for root in normalized_roots:
        normalized = root.replace("\\", "/").strip("/")
        if not normalized:
            continue
        patterns.append(re.escape(normalized).replace("/", r"[\\/]"))
    if not patterns:
        return ".*"
    return f".*({'|'.join(patterns)}).*"


def _walk_source_files(
    base_dir: Path,
    *,
    extensions: set[str],
    excluded_segments: set[str],
) -> list[Path]:
    results: list[Path] = []
    for path in base_dir.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in extensions:
            continue
        if any(segment in excluded_segments for segment in path.parts):
            continue
        results.append(path.resolve())
    return results


def _normalize_extensions(extensions: set[str]) -> set[str]:
    normalized = {suffix.lower() for suffix in extensions if suffix}
    return normalized or set(SOURCE_EXTENSIONS)


def _unique_strings(items: list[str]) -> list[str]:
    seen: set[str] = set()
    normalized_items: list[str] = []
    for item in items:
        normalized = item.strip().replace("\\", "/")
        if not normalized or normalized in seen:
            continue
        seen.add(normalized)
        normalized_items.append(normalized)
    return normalized_items


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()
