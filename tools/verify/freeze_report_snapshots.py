#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.verify.report_snapshot_support import (  # noqa: E402
    SNAPSHOT_MATRIX,
    compare_mode_for_manifest_item,
    freeze_baseline_bytes,
    sha256,
    scope_for_manifest_item,
)


def main() -> int:
    repo_root = REPO_ROOT
    source_root = (
        repo_root / "dist" / "tests" / "artifact" / "bills_tracer" / "latest" / "exports"
    )
    baseline_root = repo_root / "tests" / "golden" / "report_snapshots"

    manifest: dict[str, dict[str, str]] = {}
    copied = 0

    for key, item in SNAPSHOT_MATRIX.items():
        source_rel = item["source"]
        baseline_rel = item["baseline"]
        source_path = source_root / source_rel
        baseline_path = baseline_root / baseline_rel
        if not source_path.exists():
            print(f"[ERROR] Missing source snapshot: {source_path}")
            return 2

        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        baseline_path.write_bytes(
            freeze_baseline_bytes(
                source_path,
                compare_mode_for_manifest_item(item),
            ),
        )
        manifest[key] = {
            "source": source_rel,
            "baseline": baseline_rel,
            "scope": scope_for_manifest_item(item),
            "compare_mode": compare_mode_for_manifest_item(item),
            "sha256": sha256(baseline_path),
        }
        copied += 1

    manifest_path = baseline_root / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    print(f"[OK] Frozen {copied} snapshots.")
    print(f"[OK] Manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
