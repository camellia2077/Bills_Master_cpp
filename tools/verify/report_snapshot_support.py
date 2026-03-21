from __future__ import annotations

import hashlib
import json
from pathlib import Path

COMPARE_SCOPE_ALL = "all"
COMPARE_SCOPE_STANDARD_REPORT = "standard-report"
TEXT_COMPARE_MODE = "text"
JSON_RENDER_COMPARE_MODE = "json-render"
STANDARD_REPORT_JSON_COMPARE_MODE = "standard-report-json"

VALID_COMPARE_SCOPES = [
    COMPARE_SCOPE_ALL,
    "md",
    "json",
    "tex",
    "rst",
    "typ",
    COMPARE_SCOPE_STANDARD_REPORT,
]

SNAPSHOT_MATRIX: dict[str, dict[str, str]] = {
    "monthly_md_2025_01": {
        "source": "Markdown_bills/months/2025/2025-01.md",
        "baseline": "monthly/2025-01.md",
        "scope": "md",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "monthly_json_2025_01": {
        "source": "standard_json/months/2025/2025-01.json",
        "baseline": "monthly/2025-01.json",
        "scope": "json",
        "compare_mode": JSON_RENDER_COMPARE_MODE,
    },
    "monthly_tex_2025_01": {
        "source": "LaTeX_bills/months/2025/2025-01.tex",
        "baseline": "monthly/2025-01.tex",
        "scope": "tex",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "monthly_rst_2025_01": {
        "source": "reST_bills/months/2025/2025-01.rst",
        "baseline": "monthly/2025-01.rst",
        "scope": "rst",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "monthly_typ_2025_01": {
        "source": "Typst_bills/months/2025/2025-01.typ",
        "baseline": "monthly/2025-01.typ",
        "scope": "typ",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "monthly_standard_report_2025_01": {
        "source": "standard_json/months/2025/2025-01.json",
        "baseline": "standard_report/monthly/2025-01.json",
        "scope": COMPARE_SCOPE_STANDARD_REPORT,
        "compare_mode": STANDARD_REPORT_JSON_COMPARE_MODE,
    },
    "yearly_md_2025": {
        "source": "Markdown_bills/years/2025.md",
        "baseline": "yearly/2025.md",
        "scope": "md",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "yearly_json_2025": {
        "source": "standard_json/years/2025.json",
        "baseline": "yearly/2025.json",
        "scope": "json",
        "compare_mode": JSON_RENDER_COMPARE_MODE,
    },
    "yearly_tex_2025": {
        "source": "LaTeX_bills/years/2025.tex",
        "baseline": "yearly/2025.tex",
        "scope": "tex",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "yearly_rst_2025": {
        "source": "reST_bills/years/2025.rst",
        "baseline": "yearly/2025.rst",
        "scope": "rst",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "yearly_typ_2025": {
        "source": "Typst_bills/years/2025.typ",
        "baseline": "yearly/2025.typ",
        "scope": "typ",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "yearly_standard_report_2025": {
        "source": "standard_json/years/2025.json",
        "baseline": "standard_report/yearly/2025.json",
        "scope": COMPARE_SCOPE_STANDARD_REPORT,
        "compare_mode": STANDARD_REPORT_JSON_COMPARE_MODE,
    },
    "range_md_2025_03": {
        "source": "Markdown_bills/months/2025/2025-03.md",
        "baseline": "range/2025-03.md",
        "scope": "md",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_md_2025_04": {
        "source": "Markdown_bills/months/2025/2025-04.md",
        "baseline": "range/2025-04.md",
        "scope": "md",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_json_2025_03": {
        "source": "standard_json/months/2025/2025-03.json",
        "baseline": "range/2025-03.json",
        "scope": "json",
        "compare_mode": JSON_RENDER_COMPARE_MODE,
    },
    "range_json_2025_04": {
        "source": "standard_json/months/2025/2025-04.json",
        "baseline": "range/2025-04.json",
        "scope": "json",
        "compare_mode": JSON_RENDER_COMPARE_MODE,
    },
    "range_tex_2025_03": {
        "source": "LaTeX_bills/months/2025/2025-03.tex",
        "baseline": "range/2025-03.tex",
        "scope": "tex",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_tex_2025_04": {
        "source": "LaTeX_bills/months/2025/2025-04.tex",
        "baseline": "range/2025-04.tex",
        "scope": "tex",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_rst_2025_03": {
        "source": "reST_bills/months/2025/2025-03.rst",
        "baseline": "range/2025-03.rst",
        "scope": "rst",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_rst_2025_04": {
        "source": "reST_bills/months/2025/2025-04.rst",
        "baseline": "range/2025-04.rst",
        "scope": "rst",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_typ_2025_03": {
        "source": "Typst_bills/months/2025/2025-03.typ",
        "baseline": "range/2025-03.typ",
        "scope": "typ",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_typ_2025_04": {
        "source": "Typst_bills/months/2025/2025-04.typ",
        "baseline": "range/2025-04.typ",
        "scope": "typ",
        "compare_mode": TEXT_COMPARE_MODE,
    },
    "range_standard_report_2025_03": {
        "source": "standard_json/months/2025/2025-03.json",
        "baseline": "standard_report/range/2025-03.json",
        "scope": COMPARE_SCOPE_STANDARD_REPORT,
        "compare_mode": STANDARD_REPORT_JSON_COMPARE_MODE,
    },
    "range_standard_report_2025_04": {
        "source": "standard_json/months/2025/2025-04.json",
        "baseline": "standard_report/range/2025-04.json",
        "scope": COMPARE_SCOPE_STANDARD_REPORT,
        "compare_mode": STANDARD_REPORT_JSON_COMPARE_MODE,
    },
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def normalize_text_content(content: str) -> str:
    return content.replace("\r\n", "\n").rstrip("\n")


def _remove_generated_at_utc(payload: object) -> object:
    if isinstance(payload, dict):
        meta = payload.get("meta")
        if isinstance(meta, dict):
            meta.pop("generated_at_utc", None)
    return payload


def normalize_render_json(content: str) -> str:
    payload = _remove_generated_at_utc(json.loads(content))
    return normalize_text_content(json.dumps(payload, ensure_ascii=False, indent=2))


def normalize_standard_report_json(content: str) -> str:
    payload = _remove_generated_at_utc(json.loads(content))
    return json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def normalize_content(content: str, compare_mode: str) -> str:
    if compare_mode == TEXT_COMPARE_MODE:
        return normalize_text_content(content)
    if compare_mode == JSON_RENDER_COMPARE_MODE:
        return normalize_render_json(content)
    if compare_mode == STANDARD_REPORT_JSON_COMPARE_MODE:
        return normalize_standard_report_json(content)
    raise ValueError(f"Unsupported compare mode: {compare_mode}")


def freeze_baseline_content(source_content: str, compare_mode: str) -> str:
    if compare_mode == STANDARD_REPORT_JSON_COMPARE_MODE:
        payload = _remove_generated_at_utc(json.loads(source_content))
        return (
            json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
        )
    return source_content


def compare_mode_for_manifest_item(item: dict) -> str:
    mode = str(item.get("compare_mode", "")).strip()
    if mode:
        return mode
    suffix = Path(str(item.get("source", ""))).suffix.lower()
    if suffix == ".json":
        return JSON_RENDER_COMPARE_MODE
    return TEXT_COMPARE_MODE


def scope_for_manifest_item(item: dict) -> str:
    scope = str(item.get("scope", "")).strip().lower()
    if scope:
        return scope
    suffix = Path(str(item.get("source", ""))).suffix.lower()
    if suffix == ".md":
        return "md"
    if suffix == ".json":
        return "json"
    if suffix == ".tex":
        return "tex"
    if suffix == ".rst":
        return "rst"
    if suffix == ".typ":
        return "typ"
    return ""


def should_compare_manifest_item(item: dict, compare_scope: str) -> bool:
    if compare_scope == COMPARE_SCOPE_ALL:
        return True
    return scope_for_manifest_item(item) == compare_scope


def should_collect_extra_source(source_rel: str, compare_scope: str) -> bool:
    if compare_scope == COMPARE_SCOPE_STANDARD_REPORT:
        return False
    suffix = Path(source_rel).suffix.lower()
    if compare_scope == COMPARE_SCOPE_ALL:
        return suffix in {".md", ".json", ".tex", ".rst", ".typ"}
    if compare_scope == "md":
        return suffix == ".md"
    if compare_scope == "json":
        return suffix == ".json"
    if compare_scope == "tex":
        return suffix == ".tex"
    if compare_scope == "rst":
        return suffix == ".rst"
    if compare_scope == "typ":
        return suffix == ".typ"
    return False


def compare_mode_for_extra_source(source_rel: str) -> str:
    normalized = source_rel.replace("\\", "/")
    if normalized.endswith(".json") and (
        normalized.startswith("JSON_bills/") or normalized.startswith("standard_json/")
    ):
        return JSON_RENDER_COMPARE_MODE
    return TEXT_COMPARE_MODE
