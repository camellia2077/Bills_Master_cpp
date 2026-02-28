#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path


SNAPSHOT_MATRIX = {
    "monthly_md_2024_01": (
        "Markdown_bills/months/2024/2024-01.md",
        "monthly/2024-01.md",
    ),
    "monthly_json_2024_01": (
        "standard_json/months/2024/2024-01.json",
        "monthly/2024-01.json",
    ),
    "yearly_md_2024": (
        "Markdown_bills/years/2024.md",
        "yearly/2024.md",
    ),
    "yearly_json_2024": (
        "standard_json/years/2024.json",
        "yearly/2024.json",
    ),
    "range_md_2024_03": (
        "Markdown_bills/months/2024/2024-03.md",
        "range/2024-03.md",
    ),
    "range_md_2024_04": (
        "Markdown_bills/months/2024/2024-04.md",
        "range/2024-04.md",
    ),
    "range_json_2024_03": (
        "standard_json/months/2024/2024-03.json",
        "range/2024-03.json",
    ),
    "range_json_2024_04": (
        "standard_json/months/2024/2024-04.json",
        "range/2024-04.json",
    ),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    source_root = (
        repo_root / "test" / "output" / "artifact" / "bills_tracer" / "exported_files"
    )
    baseline_root = repo_root / "test" / "baseline" / "report_snapshots"

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
