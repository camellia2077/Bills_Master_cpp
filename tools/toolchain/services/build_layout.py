from __future__ import annotations

import hashlib
from dataclasses import dataclass
from pathlib import Path


VALID_TARGETS = ("bills", "core", "log-generator")
VALID_PRESETS = ("debug", "release", "tidy")
VALID_SCOPES = ("shared", "isolated")

LEGACY_LAYOUT_TOKENS = (
    "/".join(("tests", "output")),
    "/".join(("test", "output")),
    "_".join(("build", "fast")),
    "_".join(("build", "tidy")),
    "_".join(("build", "debug")),
)
LEGACY_FLAG_TOKENS = (
    "--" + "build-dir",
    "--" + "build-dir-mode",
)


class LegacyLayoutError(ValueError):
    pass


@dataclass(frozen=True)
class BuildDirectorySpec:
    target: str
    preset: str
    scope: str
    build_dir: Path
    cmake_build_type: str


def sanitize_segment(value: str) -> str:
    allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
    normalized = "".join(ch if ch in allowed else "_" for ch in value.strip())
    normalized = normalized.strip("_")
    return normalized or "default"


def short_hash(text: str, length: int = 12) -> str:
    return hashlib.sha1(text.encode("utf-8")).hexdigest()[:length]


def assert_no_legacy_layout(value: str, *, source: str) -> None:
    normalized = str(value).strip().replace("\\", "/")
    for token in LEGACY_LAYOUT_TOKENS:
        if token in normalized:
            raise LegacyLayoutError(
                f"{source} still uses legacy path token '{token}'. "
                "The root build-tree refactor removed that layout."
            )


def assert_no_legacy_flags(values: list[str], *, source: str) -> None:
    for item in values:
        normalized = str(item).strip()
        for token in LEGACY_FLAG_TOKENS:
            if normalized == token:
                raise LegacyLayoutError(
                    f"{source} still uses legacy flag '{token}'. "
                    "The root build-tree refactor removed that flag."
                )


def normalize_preset(value: str) -> str:
    preset = str(value).strip().lower()
    if preset not in VALID_PRESETS:
        raise ValueError(
            f"Unsupported preset '{value}'. Expected one of: {', '.join(VALID_PRESETS)}."
        )
    return preset


def normalize_scope(value: str) -> str:
    scope = str(value).strip().lower()
    if scope not in VALID_SCOPES:
        raise ValueError(
            f"Unsupported scope '{value}'. Expected one of: {', '.join(VALID_SCOPES)}."
        )
    return scope


def normalize_target(value: str) -> str:
    target = str(value).strip().lower()
    if target not in VALID_TARGETS:
        raise ValueError(
            f"Unsupported target '{value}'. Expected one of: {', '.join(VALID_TARGETS)}."
        )
    return target


def cmake_build_type_for_preset(preset: str) -> str:
    normalized = normalize_preset(preset)
    if normalized in {"debug", "tidy"}:
        return "Debug"
    return "Release"


def resolve_build_directory(
    repo_root: Path,
    *,
    target: str,
    preset: str,
    scope: str = "shared",
    instance_id: str = "",
) -> BuildDirectorySpec:
    normalized_target = normalize_target(target)
    normalized_preset = normalize_preset(preset)
    normalized_scope = normalize_scope(scope)
    build_root = (repo_root / "build").resolve()

    if normalized_target == "bills":
        if normalized_preset == "tidy" and normalized_scope != "shared":
            raise ValueError("Preset 'tidy' only supports scope 'shared'.")
        target_root = build_root / "bills" / normalized_preset
        if normalized_scope == "shared":
            build_dir = target_root / "shared"
        else:
            instance = sanitize_segment(instance_id or "manual")
            build_dir = target_root / "isolated" / instance
        return BuildDirectorySpec(
            target=normalized_target,
            preset=normalized_preset,
            scope=normalized_scope,
            build_dir=build_dir.resolve(),
            cmake_build_type=cmake_build_type_for_preset(normalized_preset),
        )

    if normalized_scope != "shared":
        raise ValueError(
            f"Target '{normalized_target}' only supports scope 'shared'."
        )

    target_folder = "log_generator" if normalized_target == "log-generator" else normalized_target
    build_dir = build_root / target_folder / normalized_preset / "shared"
    return BuildDirectorySpec(
        target=normalized_target,
        preset=normalized_preset,
        scope=normalized_scope,
        build_dir=build_dir.resolve(),
        cmake_build_type=cmake_build_type_for_preset(normalized_preset),
    )


def resolve_tests_root(repo_root: Path) -> Path:
    return (repo_root / "build" / "tests").resolve()


def resolve_runtime_project_root(repo_root: Path, project: str) -> Path:
    return (resolve_tests_root(repo_root) / "runtime" / sanitize_segment(project)).resolve()


def resolve_runtime_workspace_dir(repo_root: Path, project: str) -> Path:
    return (resolve_runtime_project_root(repo_root, project) / "workspace").resolve()


def resolve_runtime_runs_root(repo_root: Path, project: str) -> Path:
    return (resolve_runtime_project_root(repo_root, project) / "runs").resolve()


def resolve_runtime_run_dir(repo_root: Path, project: str, run_id: str) -> Path:
    return (resolve_runtime_runs_root(repo_root, project) / sanitize_segment(run_id)).resolve()


def resolve_artifact_project_root(repo_root: Path, project: str) -> Path:
    return (resolve_tests_root(repo_root) / "artifact" / sanitize_segment(project)).resolve()


def resolve_artifact_latest_dir(repo_root: Path, project: str) -> Path:
    return (resolve_artifact_project_root(repo_root, project) / "latest").resolve()


def resolve_artifact_runs_root(repo_root: Path, project: str) -> Path:
    return (resolve_artifact_project_root(repo_root, project) / "runs").resolve()


def resolve_artifact_run_dir(repo_root: Path, project: str, run_id: str) -> Path:
    return (resolve_artifact_runs_root(repo_root, project) / sanitize_segment(run_id)).resolve()


def resolve_logic_pipeline_root(repo_root: Path, pipeline_name: str) -> Path:
    return (
        resolve_tests_root(repo_root)
        / "logic"
        / "pipeline_runner"
        / sanitize_segment(pipeline_name)
    ).resolve()


def resolve_logic_pipeline_run_dir(
    repo_root: Path, pipeline_name: str, run_id: str
) -> Path:
    return (
        resolve_logic_pipeline_root(repo_root, pipeline_name)
        / "runs"
        / sanitize_segment(run_id)
    ).resolve()


def resolve_bills_master_config_path(repo_root: Path) -> Path:
    return (repo_root / "tests" / "config" / "bills_master.toml").resolve()
