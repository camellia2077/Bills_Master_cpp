from __future__ import annotations

from pathlib import Path

from .assertions import require

REPO_ROOT = Path(__file__).resolve().parents[4]


def select_fixture_txt_path() -> Path:
    fixture_root = REPO_ROOT / "testdata" / "bills"
    candidates = sorted(fixture_root.rglob("*.txt"))
    require(candidates, f"No .txt testdata bills found under: {fixture_root}")
    return candidates[0]


def parse_iso_month_from_stem(path: Path) -> tuple[int, int, str]:
    stem = path.stem
    parts = stem.split("-")
    require(len(parts) == 2, f"Fixture file must be YYYY-MM.txt: {path}")
    require(parts[0].isdigit() and len(parts[0]) == 4, f"Invalid fixture year: {path}")
    require(parts[1].isdigit() and len(parts[1]) == 2, f"Invalid fixture month: {path}")
    year = int(parts[0])
    month = int(parts[1])
    require(1900 <= year <= 9999, f"Invalid year parsed from fixture: {path}")
    require(1 <= month <= 12, f"Invalid month parsed from fixture: {path}")
    return year, month, stem
