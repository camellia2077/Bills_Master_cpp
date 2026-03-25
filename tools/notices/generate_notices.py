#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.notices.notices_support import (  # noqa: E402
    ANDROID_TARGET,
    generate_notices_outputs,
    normalize_targets,
)

DEFAULT_ANDROID_NOTICES_METADATA_DIR = (
    REPO_ROOT / "apps" / "bills_android" / "build" / "generated" / "noticesMetadata"
)
DEFAULT_ANDROID_RELEASE_ARTIFACTS = (
    DEFAULT_ANDROID_NOTICES_METADATA_DIR / "release-runtime-artifacts.json"
)
DEFAULT_ANDROID_DEBUG_ARTIFACTS = (
    DEFAULT_ANDROID_NOTICES_METADATA_DIR / "debug-runtime-artifacts.json"
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate layered and bundled open source notices artifacts.",
    )
    parser.add_argument(
        "--output-root",
        default=str(REPO_ROOT / "dist" / "notices"),
        help="Output root directory. Defaults to repo-root/dist/notices.",
    )
    parser.add_argument(
        "--targets",
        default="windows,android",
        help="Comma-separated targets to generate. Defaults to windows,android.",
    )
    parser.add_argument(
        "--android-release-artifacts",
        default="",
        help=(
            "Path to the releaseRuntimeClasspath resolved artifacts JSON. "
            "Defaults to apps/bills_android/build/generated/noticesMetadata/"
            "release-runtime-artifacts.json when present."
        ),
    )
    parser.add_argument(
        "--android-debug-artifacts",
        default="",
        help=(
            "Path to the debugRuntimeClasspath resolved artifacts JSON. "
            "Defaults to apps/bills_android/build/generated/noticesMetadata/"
            "debug-runtime-artifacts.json when present."
        ),
    )
    return parser


def _resolve_artifact_path(explicit_value: str, default_path: Path) -> Path | None:
    if explicit_value.strip():
        return Path(explicit_value).resolve()
    if default_path.exists():
        return default_path.resolve()
    return None


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        targets = normalize_targets(str(args.targets))
        android_release_artifacts_path = None
        android_debug_artifacts_path = None
        if ANDROID_TARGET in targets:
            android_release_artifacts_path = _resolve_artifact_path(
                str(args.android_release_artifacts),
                DEFAULT_ANDROID_RELEASE_ARTIFACTS,
            )
            android_debug_artifacts_path = _resolve_artifact_path(
                str(args.android_debug_artifacts),
                DEFAULT_ANDROID_DEBUG_ARTIFACTS,
            )
        outputs = generate_notices_outputs(
            REPO_ROOT,
            Path(args.output_root).resolve(),
            targets,
            android_release_artifacts_path=android_release_artifacts_path,
            android_debug_artifacts_path=android_debug_artifacts_path,
        )
    except (FileNotFoundError, ValueError, OSError) as error:
        print(f"Notices generation failed: {error}")
        return 2

    for target in targets:
        print(f"[OK] {target}: {outputs[target]}")
    for layer_id in sorted(key for key in outputs if key not in targets):
        print(f"[OK] layer {layer_id}: {outputs[layer_id]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
