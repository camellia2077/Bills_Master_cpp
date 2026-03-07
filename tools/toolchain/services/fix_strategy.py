from __future__ import annotations

from dataclasses import dataclass

from ..core.config import TidyFixStrategyConfig

STRATEGY_AUTO_FIX = "auto_fix"
STRATEGY_SAFE_REFACTOR = "safe_refactor"
STRATEGY_NOLINT_ALLOWED = "nolint_allowed"
STRATEGY_MANUAL_ONLY = "manual_only"

ALL_STRATEGIES = (
    STRATEGY_AUTO_FIX,
    STRATEGY_SAFE_REFACTOR,
    STRATEGY_NOLINT_ALLOWED,
    STRATEGY_MANUAL_ONLY,
)


@dataclass(frozen=True)
class CheckStrategySummary:
    primary_strategy: str
    safe_fix_checks_present: list[str]
    suppression_candidates_present: list[str]

_PRIMARY_PRIORITY = {
    STRATEGY_MANUAL_ONLY: 4,
    STRATEGY_SAFE_REFACTOR: 3,
    STRATEGY_AUTO_FIX: 2,
    STRATEGY_NOLINT_ALLOWED: 1,
}


def resolve_fix_strategy(check_name: str, strategy_cfg: TidyFixStrategyConfig) -> str:
    if matches_any_pattern(check_name, strategy_cfg.manual_only):
        return STRATEGY_MANUAL_ONLY
    if matches_any_pattern(check_name, strategy_cfg.auto_fix):
        return STRATEGY_AUTO_FIX
    if matches_any_pattern(check_name, strategy_cfg.safe_refactor):
        return STRATEGY_SAFE_REFACTOR
    if matches_any_pattern(check_name, strategy_cfg.nolint_allowed):
        return STRATEGY_NOLINT_ALLOWED
    return STRATEGY_MANUAL_ONLY


def resolve_primary_strategy(checks: list[str], strategy_cfg: TidyFixStrategyConfig) -> str:
    if not checks:
        return STRATEGY_MANUAL_ONLY
    strategies = {resolve_fix_strategy(check, strategy_cfg) for check in checks}
    return max(strategies, key=lambda item: _PRIMARY_PRIORITY.get(item, 0))


def summarize_checks(
    checks: list[str],
    *,
    strategy_cfg: TidyFixStrategyConfig,
    safe_fix_patterns: list[str],
    suppression_allowed_patterns: list[str],
) -> CheckStrategySummary:
    normalized_checks = [str(check).strip() for check in checks if str(check).strip()]
    return CheckStrategySummary(
        primary_strategy=resolve_primary_strategy(normalized_checks, strategy_cfg),
        safe_fix_checks_present=[
            check
            for check in normalized_checks
            if matches_any_pattern(check, safe_fix_patterns)
        ],
        suppression_candidates_present=[
            check
            for check in normalized_checks
            if matches_any_pattern(check, suppression_allowed_patterns)
        ],
    )


def matches_any_pattern(check_name: str, patterns: list[str]) -> bool:
    for pattern in patterns:
        if _match_pattern(check_name, pattern):
            return True
    return False


def _match_pattern(check_name: str, pattern: str) -> bool:
    normalized_check = check_name.strip()
    normalized_pattern = pattern.strip()
    if not normalized_check or not normalized_pattern:
        return False
    if normalized_pattern.endswith("*"):
        return normalized_check.startswith(normalized_pattern[:-1])
    return normalized_check == normalized_pattern
