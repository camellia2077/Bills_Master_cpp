from __future__ import annotations

import shutil
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover
    import tomli as tomllib  # type: ignore

WINDOWS_TARGET = "windows"
ANDROID_TARGET = "android"
TARGET_CHOICES = (WINDOWS_TARGET, ANDROID_TARGET)
CONFIG_FILENAMES = ("validator_config.toml", "modifier_config.toml", "export_formats.toml")
ANDROID_DISABLED_FORMATS = {"rst", "tex", "typ"}


def normalize_targets(raw_targets: str | list[str] | tuple[str, ...]) -> list[str]:
    if isinstance(raw_targets, str):
        candidates = [item.strip() for item in raw_targets.split(",")]
    else:
        candidates = []
        for item in raw_targets:
            candidates.extend(part.strip() for part in item.split(","))

    normalized: list[str] = []
    for candidate in candidates:
        if not candidate:
            continue
        target = candidate.lower()
        if target not in TARGET_CHOICES:
            raise ValueError(
                f"Unsupported target '{candidate}'. Supported: {', '.join(TARGET_CHOICES)}."
            )
        if target not in normalized:
            normalized.append(target)

    if not normalized:
        raise ValueError("No valid target specified.")
    return normalized


def load_enabled_formats(config_path: Path) -> list[str]:
    with config_path.open("rb") as handle:
        payload = tomllib.load(handle)

    raw_formats = payload.get("enabled_formats")
    if not isinstance(raw_formats, list) or not raw_formats:
        raise ValueError(f"'enabled_formats' must be a non-empty array in {config_path}.")

    formats: list[str] = []
    for item in raw_formats:
        if not isinstance(item, str) or not item.strip():
            raise ValueError(f"'enabled_formats' must contain non-empty strings in {config_path}.")
        formats.append(item.strip().lower())
    return formats


def render_export_formats_toml(formats: list[str]) -> str:
    joined = ", ".join(f'"{fmt}"' for fmt in formats)
    return f"enabled_formats = [{joined}]\n"


def distribute_configs(
    source_root: Path,
    output_root: Path,
    targets: str | list[str] | tuple[str, ...],
) -> dict[str, Path]:
    normalized_targets = normalize_targets(targets)
    source_root = source_root.resolve()
    output_root = output_root.resolve()

    export_formats_path = source_root / "export_formats.toml"
    enabled_formats = load_enabled_formats(export_formats_path)

    copied_outputs: dict[str, Path] = {}
    for target in normalized_targets:
        target_dir = output_root / target
        if target_dir.exists():
            shutil.rmtree(target_dir)
        target_dir.mkdir(parents=True, exist_ok=True)

        for config_name in ("validator_config.toml", "modifier_config.toml"):
            shutil.copy2(source_root / config_name, target_dir / config_name)

        if target == WINDOWS_TARGET:
            shutil.copy2(export_formats_path, target_dir / "export_formats.toml")
        else:
            filtered_formats = [
                fmt for fmt in enabled_formats if fmt not in ANDROID_DISABLED_FORMATS
            ]
            (target_dir / "export_formats.toml").write_text(
                render_export_formats_toml(filtered_formats),
                encoding="utf-8",
            )

        copied_outputs[target] = target_dir

    return copied_outputs
