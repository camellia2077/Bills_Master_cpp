from __future__ import annotations

import re
from pathlib import Path

from .models import ImportRecord, IncludeRecord

CPP_SUFFIXES = {
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".cppm",
    ".ixx",
}

CALL_LAYER_ROOTS = {
    "cli_presentation": "apps/bills_cli/src/presentation",
}

INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*([<"])\s*([^>"]+)\s*[>"]')
IMPORT_PATTERN = re.compile(r"^\s*import\b")


def scan_scope(
    repo_root: Path,
    scope_name: str,
    scope_root_relative: str,
) -> tuple[list[IncludeRecord], list[ImportRecord], int]:
    scope_root = repo_root / scope_root_relative
    if not scope_root.exists():
        return [], [], 0

    include_records: list[IncludeRecord] = []
    import_records: list[ImportRecord] = []
    scanned_files = 0

    for file_path in sorted(scope_root.rglob("*")):
        if not file_path.is_file():
            continue
        if file_path.suffix.lower() not in CPP_SUFFIXES:
            continue
        scanned_files += 1
        relative_path = file_path.resolve().relative_to(repo_root.resolve())

        try:
            lines = file_path.read_text(encoding="utf-8").splitlines()
        except OSError:
            continue

        for index, line in enumerate(lines, start=1):
            include_match = INCLUDE_PATTERN.match(line)
            if include_match is not None:
                delimiter = include_match.group(1)
                header = include_match.group(2).strip()
                include_records.append(
                    IncludeRecord(
                        scope=scope_name,
                        file_path=relative_path,
                        line=index,
                        delimiter=delimiter,
                        header=header,
                    )
                )
                continue

            if IMPORT_PATTERN.match(line) is not None:
                import_records.append(
                    ImportRecord(
                        scope=scope_name,
                        file_path=relative_path,
                        line=index,
                    )
                )

    return include_records, import_records, scanned_files


def scan_all(
    repo_root: Path,
) -> tuple[list[IncludeRecord], list[ImportRecord], dict[str, int]]:
    includes: list[IncludeRecord] = []
    imports: list[ImportRecord] = []
    scanned_file_counts: dict[str, int] = {}

    for scope_name, scope_root in CALL_LAYER_ROOTS.items():
        scope_includes, scope_imports, scanned_files = scan_scope(
            repo_root=repo_root,
            scope_name=scope_name,
            scope_root_relative=scope_root,
        )
        scanned_file_counts[scope_name] = scanned_files
        includes.extend(scope_includes)
        imports.extend(scope_imports)

    return includes, imports, scanned_file_counts


def build_observed_quoted_includes(
    include_records: list[IncludeRecord],
) -> dict[str, set[str]]:
    observed: dict[str, set[str]] = {}
    for record in include_records:
        if record.delimiter != '"':
            continue
        key = record.file_path.as_posix()
        observed.setdefault(key, set()).add(record.header)
    return observed
