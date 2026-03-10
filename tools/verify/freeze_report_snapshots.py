#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

SNAPSHOT_MATRIX = {
    "monthly_md_2025_01": (
        "Markdown_bills/months/2025/2025-01.md",
        "monthly/2025-01.md",
    ),
    "monthly_json_2025_01": (
        "standard_json/months/2025/2025-01.json",
        "monthly/2025-01.json",
    ),
    "monthly_typ_2025_01": (
        "Typst_bills/months/2025/2025-01.typ",
        "monthly/2025-01.typ",
    ),
    "yearly_md_2025": (
        "Markdown_bills/years/2025.md",
        "yearly/2025.md",
    ),
    "yearly_json_2025": (
        "standard_json/years/2025.json",
        "yearly/2025.json",
    ),
    "yearly_typ_2025": (
        "Typst_bills/years/2025.typ",
        "yearly/2025.typ",
    ),
    "range_md_2025_03": (
        "Markdown_bills/months/2025/2025-03.md",
        "range/2025-03.md",
    ),
    "range_md_2025_04": (
        "Markdown_bills/months/2025/2025-04.md",
        "range/2025-04.md",
    ),
    "range_json_2025_03": (
        "standard_json/months/2025/2025-03.json",
        "range/2025-03.json",
    ),
    "range_json_2025_04": (
        "standard_json/months/2025/2025-04.json",
        "range/2025-04.json",
    ),
    "range_typ_2025_03": (
        "Typst_bills/months/2025/2025-03.typ",
        "range/2025-03.typ",
    ),
    "range_typ_2025_04": (
        "Typst_bills/months/2025/2025-04.typ",
        "range/2025-04.typ",
    ),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    source_root = (
        repo_root / "dist" / "tests" / "artifact" / "bills_tracer" / "latest" / "exports"
    )
    baseline_root = repo_root / "tests" / "baseline" / "report_snapshots"

    manifest: dict[str, dict[str, str]] = {}
    copied = 0

    for key, (source_rel, baseline_rel) in SNAPSHOT_MATRIX.items():
        source_path = source_root / source_rel
        baseline_path = baseline_root / baseline_rel
        if not source_path.exists():
            print(f"[ERROR] Missing source snapshot: {source_path}")
            return 2

        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, baseline_path)
        manifest[key] = {
            "source": source_rel,
            "baseline": baseline_rel,
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
