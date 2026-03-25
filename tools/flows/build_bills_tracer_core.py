#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.toolchain.services.build_layout import resolve_build_directory
from tools.flows.cmake_support.models import (
    BoolOptionExpectation,
    CachePolicy,
    CMakeProjectSpec,
)
from tools.flows.cmake_support.runner import configure_and_build

PROJECT_DIR = REPO_ROOT / "libs" / "bills_core"


def build_spec(
    *,
    build_dir: Path,
    build_type: str,
    compiler: str,
    modules_enabled: bool,
    extra_args: list[str],
) -> CMakeProjectSpec:
    return CMakeProjectSpec(
        project_dir=PROJECT_DIR,
        build_dir=build_dir,
        source_dir=PROJECT_DIR,
        generator="Ninja",
        build_type=build_type,
        compiler=compiler,
        target="bills_core",
        cmake_defines=(
            "-DBILLS_CORE_BUILD_SHARED=OFF",
            f"-DBILLS_ENABLE_MODULES={'ON' if modules_enabled else 'OFF'}",
        ),
        build_args=tuple(extra_args),
    )


def build_cache_policy(*, modules_enabled: bool) -> CachePolicy:
    return CachePolicy(
        bool_options=(
            BoolOptionExpectation("BILLS_ENABLE_MODULES", modules_enabled),
            BoolOptionExpectation("BILLS_CORE_BUILD_SHARED", False),
        ),
        option_mismatch_action="reconfigure",
        skip_configure_when_cache_matches=False,
        reuse_message="==> Refreshing existing CMake configuration.",
        source_mismatch_message=(
            "==> Existing CMake cache points to a different source. Recreating dist directory."
        ),
        toolchain_mismatch_message=(
            "==> Existing CMake cache compiler/binutils selection changed. Recreating dist directory."
        ),
        option_mismatch_message=(
            "==> Existing CMake cache module options mismatch. Reconfiguring build directory in place."
        ),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Dist helper for libs/bills_core")
    parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
        help="Dist preset to use.",
    )
    parser.add_argument(
        "--compiler",
        choices=["clang"],
        default="clang",
        help="Compiler choice (clang only)",
    )
    parser.add_argument(
        "--modules",
        dest="modules_enabled",
        action="store_true",
        default=False,
        help="Enable C++ modules pilot (BILLS_ENABLE_MODULES=ON).",
    )
    parser.add_argument(
        "--no-modules",
        dest="modules_enabled",
        action="store_false",
        help="Disable C++ modules pilot (BILLS_ENABLE_MODULES=OFF).",
    )
    args, extra_args = parser.parse_known_args()
    if extra_args and extra_args[0] == "--":
        extra_args = extra_args[1:]
    if any(flag in {"--shared", "--static"} for flag in extra_args):
        print("!!! Error: Windows/core dist no longer accepts --shared or --static.")
        return 2

    spec = resolve_build_directory(
        REPO_ROOT,
        target="bills-tracer-core",
        preset=args.preset,
        scope="shared",
    )
    configure_and_build(
        build_spec(
            build_dir=spec.build_dir,
            build_type=spec.cmake_build_type,
            compiler=args.compiler,
            modules_enabled=args.modules_enabled,
            extra_args=extra_args,
        ),
        build_cache_policy(modules_enabled=args.modules_enabled),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
