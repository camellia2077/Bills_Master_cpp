#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


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
    "core_application": "libs/bills_core/src/application",
    "core_reports_core": "libs/bills_core/src/reports/core",
    "core_abi_handlers": "libs/bills_core/src/abi/internal/handlers",
    "cli_presentation": "apps/bills_cli/src/windows/presentation",
}

INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*([<"])\s*([^>"]+)\s*[>"]')
IMPORT_PATTERN = re.compile(r"^\s*import\b")


@dataclass(frozen=True)
class IncludeRecord:
    scope: str
    file_path: Path
    line: int
    delimiter: str
    header: str


@dataclass(frozen=True)
class ImportRecord:
    scope: str
    file_path: Path
    line: int


@dataclass(frozen=True)
class Violation:
    file_path: Path
    line: int
    header: str
    reason: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check call-layer import/include policy: quoted includes in call-layer "
            "files must be explicitly allowlisted."
        )
    )
    parser.add_argument(
        "--config",
        default="tools/verify/tools/config/import_layer_whitelist.json",
        help="Allowlist config path (relative to repo root by default).",
    )
    parser.add_argument(
        "--stats",
        action="store_true",
        help="Print include/import distribution stats for call-layer roots.",
    )
    parser.add_argument(
        "--write-baseline",
        action="store_true",
        help="Regenerate allowlist baseline from current quoted includes and exit.",
    )
    parser.add_argument(
        "--baseline-owner",
        default="phase0-baseline",
        help="Default owner for newly generated baseline entries.",
    )
    parser.add_argument(
        "--baseline-reason",
        default="Phase0 baseline keep; pending migration to import",
        help="Default reason for newly generated baseline entries.",
    )
    return parser.parse_args()


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


def normalize_config_path(repo_root: Path, config_arg: str) -> Path:
    config_path = Path(config_arg)
    if config_path.is_absolute():
        return config_path
    return (repo_root / config_path).resolve()


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


def load_allowlist(
    config_path: Path,
) -> tuple[set[tuple[str, str]], list[str]]:
    if not config_path.exists():
        return set(), [f"Allowlist config does not exist: {config_path}"]

    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return set(), [f"Invalid allowlist config JSON: {config_path} ({exc})"]

    allowed = set()
    errors: list[str] = []
    raw_allowlist = content.get("allowed_quoted_includes")
    if not isinstance(raw_allowlist, dict):
        return set(), ["allowlist schema error: 'allowed_quoted_includes' must be object"]

    for file_path, entries in raw_allowlist.items():
        if not isinstance(file_path, str):
            errors.append("allowlist schema error: file path key must be string")
            continue
        if not isinstance(entries, list):
            errors.append(
                f"allowlist schema error: entries for '{file_path}' must be list"
            )
            continue

        for entry in entries:
            if not isinstance(entry, dict):
                errors.append(
                    f"allowlist schema error: entry for '{file_path}' must be object"
                )
                continue

            header = entry.get("header")
            owner = entry.get("owner")
            reason = entry.get("reason")
            if not isinstance(header, str) or not header:
                errors.append(
                    f"allowlist schema error: entry for '{file_path}' missing header"
                )
                continue
            if not isinstance(owner, str) or not owner.strip():
                errors.append(
                    f"allowlist schema error: '{file_path}' -> '{header}' missing owner"
                )
            if not isinstance(reason, str) or not reason.strip():
                errors.append(
                    f"allowlist schema error: '{file_path}' -> '{header}' missing reason"
                )

            allowed.add((file_path.replace("\\", "/"), header))

    return allowed, errors


def existing_metadata(
    config_path: Path,
) -> dict[tuple[str, str], dict[str, str]]:
    if not config_path.exists():
        return {}
    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}

    result: dict[tuple[str, str], dict[str, str]] = {}
    raw_allowlist = content.get("allowed_quoted_includes", {})
    if not isinstance(raw_allowlist, dict):
        return {}

    for file_path, entries in raw_allowlist.items():
        if not isinstance(file_path, str) or not isinstance(entries, list):
            continue
        normalized_path = file_path.replace("\\", "/")
        for entry in entries:
            if not isinstance(entry, dict):
                continue
            header = entry.get("header")
            if not isinstance(header, str) or not header:
                continue
            owner = entry.get("owner")
            reason = entry.get("reason")
            result[(normalized_path, header)] = {
                "owner": owner if isinstance(owner, str) and owner.strip() else "",
                "reason": reason if isinstance(reason, str) and reason.strip() else "",
            }
    return result


def write_baseline(
    repo_root: Path,
    config_path: Path,
    include_records: list[IncludeRecord],
    baseline_owner: str,
    baseline_reason: str,
) -> None:
    observed = build_observed_quoted_includes(include_records)
    preserved = existing_metadata(config_path)

    ordered_allowlist: dict[str, list[dict[str, str]]] = {}
    for file_path in sorted(observed.keys()):
        headers = sorted(observed[file_path])
        items: list[dict[str, str]] = []
        for header in headers:
            meta = preserved.get((file_path, header), {})
            owner = meta.get("owner") or baseline_owner
            reason = meta.get("reason") or baseline_reason
            items.append(
                {
                    "header": header,
                    "owner": owner,
                    "reason": reason,
                }
            )
        ordered_allowlist[file_path] = items

    payload = {
        "schema_version": 1,
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "policy": (
            "Call-layer files should default to import. "
            "Quoted includes must be explicitly allowlisted."
        ),
        "call_layer_roots": CALL_LAYER_ROOTS,
        "allowed_quoted_includes": ordered_allowlist,
    }

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    print(
        "[OK] import-layer allowlist baseline generated: "
        f"{config_path} (entries={sum(len(v) for v in ordered_allowlist.values())})"
    )


def print_stats(
    include_records: list[IncludeRecord],
    import_records: list[ImportRecord],
    scanned_file_counts: dict[str, int],
) -> None:
    include_by_scope: dict[str, int] = {}
    quoted_include_by_scope: dict[str, int] = {}
    import_by_scope: dict[str, int] = {}

    for item in include_records:
        include_by_scope[item.scope] = include_by_scope.get(item.scope, 0) + 1
        if item.delimiter == '"':
            quoted_include_by_scope[item.scope] = (
                quoted_include_by_scope.get(item.scope, 0) + 1
            )
    for item in import_records:
        import_by_scope[item.scope] = import_by_scope.get(item.scope, 0) + 1

    print("[INFO] import/include distribution (call-layer roots)")
    print("scope,files,include_total,include_quoted,import_total")
    for scope_name in CALL_LAYER_ROOTS.keys():
        file_count = scanned_file_counts.get(scope_name, 0)
        include_total = include_by_scope.get(scope_name, 0)
        include_quoted = quoted_include_by_scope.get(scope_name, 0)
        import_total = import_by_scope.get(scope_name, 0)
        print(
            f"{scope_name},{file_count},{include_total},{include_quoted},{import_total}"
        )


def check_policy(
    include_records: list[IncludeRecord],
    allowlist: set[tuple[str, str]],
) -> tuple[list[Violation], set[tuple[str, str]]]:
    violations: list[Violation] = []
    observed = set()

    for record in include_records:
        if record.delimiter != '"':
            continue

        key = (record.file_path.as_posix(), record.header)
        observed.add(key)
        if key in allowlist:
            continue

        violations.append(
            Violation(
                file_path=record.file_path,
                line=record.line,
                header=record.header,
                reason=(
                    "调用层新增了未登记的 quoted include。"
                    "请改为 import，或在 allowlist 中补充 owner/reason。"
                ),
            )
        )

    return violations, observed


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[3]
    config_path = normalize_config_path(repo_root, args.config)

    include_records, import_records, scanned_file_counts = scan_all(repo_root)
    if args.stats:
        print_stats(include_records, import_records, scanned_file_counts)

    if args.write_baseline:
        write_baseline(
            repo_root=repo_root,
            config_path=config_path,
            include_records=include_records,
            baseline_owner=args.baseline_owner,
            baseline_reason=args.baseline_reason,
        )
        return 0

    allowlist, schema_errors = load_allowlist(config_path)
    if schema_errors:
        print("[FAILED] import-layer-check: invalid allowlist config")
        for error in schema_errors:
            print(f"- {error}")
        return 2

    violations, observed = check_policy(include_records, allowlist)
    stale = sorted(allowlist - observed)

    if violations:
        print(
            "[FAILED] import-layer-check failed: "
            f"violations={len(violations)}, scanned_files={sum(scanned_file_counts.values())}"
        )
        for item in violations:
            print(
                f"- {item.file_path.as_posix()}:{item.line} "
                f"#include \"{item.header}\" -> {item.reason}"
            )
        return 1

    print(
        "[OK] import-layer-check passed: "
        f"quoted_includes={len(observed)}, scanned_files={sum(scanned_file_counts.values())}"
    )
    if stale:
        print(f"[WARN] stale allowlist entries detected: {len(stale)}")
        for file_path, header in stale[:20]:
            print(f"- {file_path} -> {header}")
        if len(stale) > 20:
            print(f"... ({len(stale) - 20} more)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

