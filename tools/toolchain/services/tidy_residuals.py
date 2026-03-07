from __future__ import annotations

import json
from pathlib import Path

from .fix_strategy import matches_any_pattern

PREFERRED_MANUAL_REFACTOR = "manual_refactor"
PREFERRED_SUGGEST_NOLINT = "suggest_nolint"


def classify_residual_diagnostics(
    diagnostics: list[dict],
    *,
    suppression_allowed_patterns: list[str],
    safe_fix_patterns: list[str],
    count_safe_fix_as_unexpected: bool = True,
) -> tuple[list[dict], dict]:
    classified: list[dict] = []
    manual_refactor_count = 0
    suggest_nolint_count = 0
    unexpected_fixable_count = 0
    files_with_remaining: set[str] = set()

    for item in diagnostics:
        check_name = str(item.get("check", "")).strip()
        preferred_action = _preferred_action(
            check_name, suppression_allowed_patterns=suppression_allowed_patterns
        )
        fallback_action = _fallback_action(
            check_name,
            preferred_action=preferred_action,
            suppression_allowed_patterns=suppression_allowed_patterns,
        )
        reason_template = _reason_template(check_name, preferred_action)
        file_path = str(item.get("file", "")).strip()
        if file_path:
            files_with_remaining.add(file_path)
        if preferred_action == PREFERRED_SUGGEST_NOLINT:
            suggest_nolint_count += 1
        else:
            manual_refactor_count += 1
        if (
            count_safe_fix_as_unexpected
            and matches_any_pattern(check_name, safe_fix_patterns)
        ):
            unexpected_fixable_count += 1
        classified.append(
            {
                "file": file_path,
                "line": int(item.get("line", 0) or 0),
                "col": int(item.get("col", 0) or 0),
                "check": check_name,
                "message": str(item.get("message", "")).strip(),
                "severity": str(item.get("severity", "warning")).strip() or "warning",
                "preferred_action": preferred_action,
                "fallback_action": fallback_action,
                "reason_template": reason_template,
            }
        )

    summary = {
        "manual_refactor_count": manual_refactor_count,
        "suggest_nolint_count": suggest_nolint_count,
        "unexpected_fixable_count": unexpected_fixable_count,
        "files_with_remaining": sorted(files_with_remaining),
    }
    return classified, summary


def load_diagnostics_jsonl(path: Path) -> list[dict]:
    if not path.exists():
        return []
    diagnostics: list[dict] = []
    try:
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            raw = line.strip()
            if not raw:
                continue
            payload = json.loads(raw)
            if isinstance(payload, dict):
                diagnostics.append(payload)
    except (OSError, json.JSONDecodeError):
        return []
    return diagnostics


def _preferred_action(
    check_name: str,
    *,
    suppression_allowed_patterns: list[str],
) -> str:
    if matches_any_pattern(check_name, suppression_allowed_patterns):
        return PREFERRED_SUGGEST_NOLINT
    return PREFERRED_MANUAL_REFACTOR


def _fallback_action(
    check_name: str,
    *,
    preferred_action: str,
    suppression_allowed_patterns: list[str],
) -> str | None:
    if preferred_action == PREFERRED_SUGGEST_NOLINT:
        return None
    if matches_any_pattern(
        check_name,
        [
            "readability-function-cognitive-complexity",
            "bugprone-easily-swappable-parameters",
        ],
    ):
        return PREFERRED_SUGGEST_NOLINT
    if matches_any_pattern(check_name, suppression_allowed_patterns):
        return PREFERRED_SUGGEST_NOLINT
    return None


def _reason_template(check_name: str, preferred_action: str) -> str:
    if check_name == "readability-function-cognitive-complexity":
        return "external behavior preserved; local extraction too invasive for current batch"
    if check_name == "bugprone-easily-swappable-parameters":
        return "signature retained for caller compatibility in current batch"
    if "magic-numbers" in check_name:
        return "domain constant should be extracted before suppression"
    if preferred_action == PREFERRED_SUGGEST_NOLINT:
        return "narrow local suppression allowed by tidy suppression policy"
    return "manual source change required; no narrow suppression rule is configured"
