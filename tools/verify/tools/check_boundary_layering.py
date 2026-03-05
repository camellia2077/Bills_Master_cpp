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

BOUNDARY_LAYER_ROOTS = {
    "core_abi": "libs/bills_core/src/abi",
    "core_modules_bridge": "libs/bills_core/src/modules",
    "io_adapters": "libs/bills_io/src/bills_io/adapters",
}

INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*([<"])\s*([^>"]+)\s*[>"]')


@dataclass(frozen=True)
class IncludeRecord:
    scope: str
    file_path: Path
    line: int
    delimiter: str
    header: str


@dataclass(frozen=True)
class Violation:
    file_path: Path
    line: int
    header: str
    reason: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check boundary-layer include allowlist policy: quoted includes in "
            "boundary files must be explicitly allowlisted with owner/reason/window/tier."
        )
    )
    parser.add_argument(
        "--config",
        default="tools/verify/tools/config/boundary_include_whitelist.json",
        help="Boundary allowlist config path (relative to repo root by default).",
    )
    parser.add_argument(
        "--stats",
        action="store_true",
        help="Print include distribution stats for boundary roots.",
    )
    parser.add_argument(
        "--write-baseline",
        action="store_true",
        help="Regenerate boundary allowlist baseline from current quoted includes and exit.",
    )
    parser.add_argument(
        "--baseline-owner",
        default="phase4-boundary-whitelist",
        help="Default owner for newly generated baseline entries.",
    )
    return parser.parse_args()


def normalize_config_path(repo_root: Path, config_arg: str) -> Path:
    config_path = Path(config_arg)
    if config_path.is_absolute():
        return config_path
    return (repo_root / config_path).resolve()


def scan_scope(
    repo_root: Path,
    scope_name: str,
    scope_root_relative: str,
) -> tuple[list[IncludeRecord], int]:
    scope_root = repo_root / scope_root_relative
    if not scope_root.exists():
        return [], 0

    include_records: list[IncludeRecord] = []
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
            if include_match is None:
                continue
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

    return include_records, scanned_files


def scan_all(repo_root: Path) -> tuple[list[IncludeRecord], dict[str, int]]:
    includes: list[IncludeRecord] = []
    scanned_file_counts: dict[str, int] = {}

    for scope_name, scope_root in BOUNDARY_LAYER_ROOTS.items():
        scope_includes, scanned_files = scan_scope(
            repo_root=repo_root,
            scope_name=scope_name,
            scope_root_relative=scope_root,
        )
        scanned_file_counts[scope_name] = scanned_files
        includes.extend(scope_includes)

    return includes, scanned_file_counts


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


def classify_entry(file_path: str, header: str) -> tuple[str, str]:
    if (
        file_path.startswith("libs/bills_io/src/bills_io/adapters/")
        or header == "nlohmann/json.hpp"
        or header == "abi/bills_core_abi.h"
    ):
        return (
            "long-term",
            "长期保留（第三方/平台适配或 ABI 对外契约，不计划在迁移窗口内移除）",
        )

    if file_path.startswith("libs/bills_core/src/modules/"):
        return (
            "replaceable",
            "Phase 5.2（编译器兼容矩阵稳定后，评估 header-unit/纯 import 替代桥接 include）",
        )

    return (
        "replaceable",
        "Phase 5.1（模块默认 ON 并保留 OFF 回退期后，评估继续压缩 ABI 边界 include）",
    )


def reason_for_entry(file_path: str, header: str, tier: str) -> str:
    if tier == "long-term":
        if header == "abi/bills_core_abi.h":
            return "C ABI 对外导出头，属于稳定边界契约。"
        if header == "nlohmann/json.hpp":
            return "JSON 协议解析依赖第三方库，边界层直接保留。"
        if file_path.startswith("libs/bills_io/src/bills_io/adapters/"):
            return "IO 适配器属于平台/第三方边界实现，允许保留 include。"
        return "边界层长期依赖，短期内无替换收益。"

    if file_path.startswith("libs/bills_core/src/modules/"):
        return "模块桥接段用于连接遗留头与 export 接口，后续可继续收敛。"
    if file_path.startswith("libs/bills_core/src/abi/"):
        return "ABI 命令处理与共享边界实现当前需要该 include，后续继续迁移为更小边界。"
    return "边界层临时依赖，后续在迁移窗口内持续压缩。"


def existing_metadata(config_path: Path) -> dict[tuple[str, str], dict[str, str]]:
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
            result[(normalized_path, header)] = {
                "owner": str(entry.get("owner", "")).strip(),
                "reason": str(entry.get("reason", "")).strip(),
                "window": str(entry.get("window", "")).strip(),
                "tier": str(entry.get("tier", "")).strip(),
            }
    return result


def write_baseline(
    config_path: Path,
    include_records: list[IncludeRecord],
    baseline_owner: str,
) -> None:
    observed = build_observed_quoted_includes(include_records)
    preserved = existing_metadata(config_path)

    ordered_allowlist: dict[str, list[dict[str, str]]] = {}
    for file_path in sorted(observed.keys()):
        headers = sorted(observed[file_path])
        items: list[dict[str, str]] = []
        for header in headers:
            key = (file_path, header)
            current_meta = preserved.get(key, {})
            tier, window = classify_entry(file_path, header)
            tier = current_meta.get("tier") or tier
            window = current_meta.get("window") or window
            reason = current_meta.get("reason") or reason_for_entry(
                file_path=file_path,
                header=header,
                tier=tier,
            )
            owner = current_meta.get("owner") or baseline_owner

            items.append(
                {
                    "header": header,
                    "owner": owner,
                    "reason": reason,
                    "window": window,
                    "tier": tier,
                }
            )
        ordered_allowlist[file_path] = items

    payload = {
        "schema_version": 1,
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "policy": (
            "Boundary layer allows quoted includes, but each entry must define "
            "owner/reason/window/tier (replaceable vs long-term)."
        ),
        "boundary_layer_roots": BOUNDARY_LAYER_ROOTS,
        "allowed_quoted_includes": ordered_allowlist,
    }

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    total_entries = sum(len(items) for items in ordered_allowlist.values())
    print(
        "[OK] boundary include allowlist baseline generated: "
        f"{config_path} (entries={total_entries})"
    )


def load_allowlist(
    config_path: Path,
) -> tuple[set[tuple[str, str]], list[str], dict[tuple[str, str], str]]:
    if not config_path.exists():
        return set(), [f"Allowlist config does not exist: {config_path}"], {}

    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return set(), [f"Invalid allowlist config JSON: {config_path} ({exc})"], {}

    allowed = set()
    errors: list[str] = []
    tier_map: dict[tuple[str, str], str] = {}
    raw_allowlist = content.get("allowed_quoted_includes")
    if not isinstance(raw_allowlist, dict):
        return (
            set(),
            ["allowlist schema error: 'allowed_quoted_includes' must be object"],
            {},
        )

    valid_tiers = {"replaceable", "long-term"}
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
            window = entry.get("window")
            tier = entry.get("tier")

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
            if not isinstance(window, str) or not window.strip():
                errors.append(
                    f"allowlist schema error: '{file_path}' -> '{header}' missing window"
                )
            if not isinstance(tier, str) or tier not in valid_tiers:
                errors.append(
                    f"allowlist schema error: '{file_path}' -> '{header}' tier must be one of {sorted(valid_tiers)}"
                )
                tier = "replaceable"

            key = (file_path.replace("\\", "/"), header)
            allowed.add(key)
            tier_map[key] = tier

    return allowed, errors, tier_map


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
                    "边界层新增了未登记的 quoted include。"
                    "请补充 allowlist 的 owner/reason/window/tier。"
                ),
            )
        )

    return violations, observed


def print_stats(
    include_records: list[IncludeRecord],
    scanned_file_counts: dict[str, int],
    observed: set[tuple[str, str]] | None = None,
    tier_map: dict[tuple[str, str], str] | None = None,
) -> None:
    include_by_scope: dict[str, int] = {}
    quoted_include_by_scope: dict[str, int] = {}

    for item in include_records:
        include_by_scope[item.scope] = include_by_scope.get(item.scope, 0) + 1
        if item.delimiter == '"':
            quoted_include_by_scope[item.scope] = (
                quoted_include_by_scope.get(item.scope, 0) + 1
            )

    print("[INFO] include distribution (boundary roots)")
    print("scope,files,include_total,include_quoted")
    for scope_name in BOUNDARY_LAYER_ROOTS.keys():
        file_count = scanned_file_counts.get(scope_name, 0)
        include_total = include_by_scope.get(scope_name, 0)
        include_quoted = quoted_include_by_scope.get(scope_name, 0)
        print(f"{scope_name},{file_count},{include_total},{include_quoted}")

    if observed is not None and tier_map is not None:
        long_term = sum(1 for key in observed if tier_map.get(key) == "long-term")
        replaceable = sum(1 for key in observed if tier_map.get(key) == "replaceable")
        print(
            "[INFO] boundary allowlist tier stats: "
            f"replaceable={replaceable}, long-term={long_term}"
        )


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[3]
    config_path = normalize_config_path(repo_root, args.config)

    include_records, scanned_file_counts = scan_all(repo_root)

    if args.write_baseline:
        write_baseline(
            config_path=config_path,
            include_records=include_records,
            baseline_owner=args.baseline_owner,
        )
        return 0

    allowlist, schema_errors, tier_map = load_allowlist(config_path)
    if schema_errors:
        print("[FAILED] boundary-layer-check: invalid allowlist config")
        for error in schema_errors:
            print(f"- {error}")
        return 2

    violations, observed = check_policy(include_records, allowlist)
    if args.stats:
        print_stats(include_records, scanned_file_counts, observed, tier_map)

    stale = sorted(allowlist - observed)
    if violations:
        print(
            "[FAILED] boundary-layer-check failed: "
            f"violations={len(violations)}, scanned_files={sum(scanned_file_counts.values())}"
        )
        for item in violations:
            print(
                f"- {item.file_path.as_posix()}:{item.line} "
                f"#include \"{item.header}\" -> {item.reason}"
            )
        return 1

    print(
        "[OK] boundary-layer-check passed: "
        f"quoted_includes={len(observed)}, scanned_files={sum(scanned_file_counts.values())}"
    )
    if stale:
        print(f"[WARN] stale boundary allowlist entries detected: {len(stale)}")
        for file_path, header in stale[:20]:
            print(f"- {file_path} -> {header}")
        if len(stale) > 20:
            print(f"... ({len(stale) - 20} more)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
