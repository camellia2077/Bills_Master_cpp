from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class DistConfig:
    default_target: str = "bills-tracer-cli"


@dataclass
class ScopeConfig:
    default_roots: list[str] = field(
        default_factory=lambda: [
            "apps/bills_cli/src",
            "libs/bills_core/src",
            "libs/bills_io/src",
        ]
    )
    optional_roots: list[str] = field(
        default_factory=lambda: [
            "tests/generators/log_generator/src",
        ]
    )
    exclude_segments: list[str] = field(
        default_factory=lambda: [
            ".git",
            "dist",
            "temp",
        ]
    )
    header_filter_roots: list[str] = field(
        default_factory=lambda: [
            "apps/bills_cli/src",
            "libs/bills_core/src",
            "libs/bills_io/src",
        ]
    )


@dataclass
class TidyFixStrategyConfig:
    auto_fix: list[str] = field(
        default_factory=lambda: [
            "modernize-use-nullptr",
            "modernize-use-override",
            "modernize-use-using",
            "modernize-loop-convert",
            "modernize-raw-string-literal",
            "readability-braces-around-statements",
            "readability-else-after-return",
            "readability-redundant-control-flow",
        ]
    )
    safe_refactor: list[str] = field(
        default_factory=lambda: [
            "bugprone-easily-swappable-parameters",
            "readability-function-cognitive-complexity",
            "bugprone-branch-clone",
            "performance-for-range-copy",
            "modernize-pass-by-value",
            "readability-identifier-naming",
        ]
    )
    nolint_allowed: list[str] = field(
        default_factory=lambda: [
            "readability-function-cognitive-complexity",
            "bugprone-easily-swappable-parameters",
            "cppcoreguidelines-avoid-magic-numbers",
        ]
    )
    manual_only: list[str] = field(
        default_factory=lambda: [
            "clang-diagnostic-error",
            "clang-diagnostic-*",
            "clang-analyzer-*",
            "cppcoreguidelines-pro-type-reinterpret-cast",
            "concurrency-*",
        ]
    )


@dataclass
class TidySafeFixPrepassConfig:
    checks: list[str] = field(
        default_factory=lambda: [
            "modernize-use-nullptr",
            "modernize-use-override",
            "modernize-use-using",
            "modernize-loop-convert",
            "modernize-raw-string-literal",
            "readability-braces-around-statements",
            "readability-else-after-return",
            "readability-redundant-control-flow",
            "modernize-use-trailing-return-type",
            "modernize-use-nodiscard",
        ]
    )


@dataclass
class TidySuppressionConfig:
    mode: str = "suggest_only"
    allowed_checks: list[str] = field(
        default_factory=lambda: [
            "readability-function-cognitive-complexity",
            "bugprone-easily-swappable-parameters",
            "cppcoreguidelines-avoid-magic-numbers",
        ]
    )


@dataclass
class TidyStatusConfig:
    explain_closed_ranges: bool = True


@dataclass
class TidyConfig:
    max_lines: int = 150
    max_diags: int = 12
    batch_size: int = 10
    jobs: int = 0
    full_every: int = 3
    neighbor_scope: str = "none"
    keep_going: bool = True
    auto_full_on_no_such_file: bool = True
    auto_full_on_glob_mismatch: bool = True
    fix_strategy: TidyFixStrategyConfig = field(default_factory=TidyFixStrategyConfig)
    safe_fix_prepass: TidySafeFixPrepassConfig = field(default_factory=TidySafeFixPrepassConfig)
    suppression: TidySuppressionConfig = field(default_factory=TidySuppressionConfig)
    status: TidyStatusConfig = field(default_factory=TidyStatusConfig)


@dataclass
class ToolchainConfig:
    dist: DistConfig = field(default_factory=DistConfig)
    scope: ScopeConfig = field(default_factory=ScopeConfig)
    tidy: TidyConfig = field(default_factory=TidyConfig)


def load_toolchain_config(config_path: Path) -> ToolchainConfig:
    data = _load_toml_dict(config_path)
    if "build" in data:
        raise ValueError(
            "Legacy config section [build] is no longer supported. Rename it to [dist]."
        )
    dist_data = data.get("dist", {})
    scope_data = data.get("scope", {})
    tidy_data = data.get("tidy", {})
    fix_strategy_data = tidy_data.get("fix_strategy", {})
    safe_fix_prepass_data = tidy_data.get("safe_fix_prepass", {})
    suppression_data = tidy_data.get("suppression", {})
    status_data = tidy_data.get("status", {})

    config = ToolchainConfig()
    if isinstance(dist_data, dict):
        default_target = dist_data.get("default_target")
        if isinstance(default_target, str) and default_target.strip():
            config.dist.default_target = default_target.strip()

    if isinstance(scope_data, dict):
        config.scope.default_roots = _get_str_list(
            scope_data, "default_roots", config.scope.default_roots
        )
        config.scope.optional_roots = _get_str_list(
            scope_data,
            "optional_roots",
            config.scope.optional_roots,
            allow_empty=True,
        )
        config.scope.exclude_segments = _get_str_list(
            scope_data,
            "exclude_segments",
            config.scope.exclude_segments,
            allow_empty=True,
        )
        header_filter_roots = _get_str_list(
            scope_data,
            "header_filter_roots",
            config.scope.default_roots,
            allow_empty=True,
        )
        config.scope.header_filter_roots = header_filter_roots or list(config.scope.default_roots)

    if isinstance(tidy_data, dict):
        config.tidy.max_lines = _get_int(tidy_data, "max_lines", config.tidy.max_lines)
        config.tidy.max_diags = _get_int(tidy_data, "max_diags", config.tidy.max_diags)
        config.tidy.batch_size = _get_int(tidy_data, "batch_size", config.tidy.batch_size)
        config.tidy.jobs = _get_int(tidy_data, "jobs", config.tidy.jobs)
        config.tidy.full_every = _get_int(tidy_data, "full_every", config.tidy.full_every)
        config.tidy.neighbor_scope = _get_str(
            tidy_data, "neighbor_scope", config.tidy.neighbor_scope
        )
        config.tidy.keep_going = _get_bool(tidy_data, "keep_going", config.tidy.keep_going)
        config.tidy.auto_full_on_no_such_file = _get_bool(
            tidy_data,
            "auto_full_on_no_such_file",
            config.tidy.auto_full_on_no_such_file,
        )
        config.tidy.auto_full_on_glob_mismatch = _get_bool(
            tidy_data,
            "auto_full_on_glob_mismatch",
            config.tidy.auto_full_on_glob_mismatch,
        )

    if isinstance(fix_strategy_data, dict):
        config.tidy.fix_strategy.auto_fix = _get_str_list(
            fix_strategy_data, "auto_fix", config.tidy.fix_strategy.auto_fix
        )
        config.tidy.fix_strategy.safe_refactor = _get_str_list(
            fix_strategy_data, "safe_refactor", config.tidy.fix_strategy.safe_refactor
        )
        config.tidy.fix_strategy.nolint_allowed = _get_str_list(
            fix_strategy_data, "nolint_allowed", config.tidy.fix_strategy.nolint_allowed
        )
        config.tidy.fix_strategy.manual_only = _get_str_list(
            fix_strategy_data, "manual_only", config.tidy.fix_strategy.manual_only
        )

    if isinstance(safe_fix_prepass_data, dict):
        config.tidy.safe_fix_prepass.checks = _get_str_list(
            safe_fix_prepass_data,
            "checks",
            config.tidy.safe_fix_prepass.checks,
        )

    if isinstance(suppression_data, dict):
        config.tidy.suppression.mode = _get_str(
            suppression_data, "mode", config.tidy.suppression.mode
        )
        config.tidy.suppression.allowed_checks = _get_str_list(
            suppression_data,
            "allowed_checks",
            config.tidy.suppression.allowed_checks,
        )

    if isinstance(status_data, dict):
        config.tidy.status.explain_closed_ranges = _get_bool(
            status_data,
            "explain_closed_ranges",
            config.tidy.status.explain_closed_ranges,
        )

    return config


def _load_toml_dict(config_path: Path) -> dict:
    if not config_path.exists():
        return {}
    try:
        import tomllib  # py311+
    except ImportError as exc:  # pragma: no cover
        raise RuntimeError("Python 3.11+ is required for tomllib support.") from exc
    try:
        with config_path.open("rb") as handle:
            payload = tomllib.load(handle)
    except OSError as exc:
        raise RuntimeError(f"Failed to read toolchain config: {config_path}") from exc
    if not isinstance(payload, dict):
        return {}
    return payload


def _get_int(data: dict, key: str, default: int) -> int:
    value = data.get(key)
    return value if isinstance(value, int) else default


def _get_bool(data: dict, key: str, default: bool) -> bool:
    value = data.get(key)
    return value if isinstance(value, bool) else default


def _get_str(data: dict, key: str, default: str) -> str:
    value = data.get(key)
    if isinstance(value, str) and value.strip():
        return value.strip()
    return default


def _get_str_list(
    data: dict,
    key: str,
    default: list[str],
    *,
    allow_empty: bool = False,
) -> list[str]:
    value = data.get(key)
    if not isinstance(value, list):
        return list(default)
    items: list[str] = []
    for item in value:
        if isinstance(item, str) and item.strip():
            items.append(item.strip())
    if items:
        return items
    if allow_empty:
        return []
    return list(default)
