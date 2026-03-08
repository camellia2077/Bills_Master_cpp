from __future__ import annotations

import argparse
from pathlib import Path

from .metadata import load_allowlist, normalize_config_path, write_baseline
from .policy import check_policy, print_stats
from .scan import scan_all


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


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[4]
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
                f'#include "{item.header}" -> {item.reason}'
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
