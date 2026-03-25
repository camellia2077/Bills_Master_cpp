#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.toolchain.services.build_layout import (
    resolve_build_directory,
    resolve_runtime_workspace_dir,
)
from tools.toolchain.core.path_ops import replace_path
from tools.flows.bills_tracer_flow_support.config_distribution import (
    WINDOWS_TARGET,
    distribute_configs,
)
from tools.notices.notices_support import generate_notices_outputs
from tools.flows.cmake_support.models import (
    BoolOptionExpectation,
    CachePolicy,
    CMakeProjectSpec,
)
from tools.flows.cmake_support.runner import configure_and_build

PROJECT_DIR = REPO_ROOT / "apps" / "bills_cli"
RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".manifest", ".pdb"}
def sync_runtime_artifacts(build_dir: Path) -> None:
    build_bin_dir = build_dir / "bin"
    if not build_bin_dir.exists():
        print(f"==> Skip artifact sync: missing directory {build_bin_dir}")
        return

    runtime_workspace_dir = resolve_runtime_workspace_dir(REPO_ROOT, "bills_tracer")
    runtime_workspace_dir.mkdir(parents=True, exist_ok=True)
    for stale in runtime_workspace_dir.iterdir():
        if stale.is_file() and stale.suffix.lower() in RUNTIME_SIDECAR_EXTS:
            stale.unlink()

    copied_files: list[str] = []
    for entry in sorted(build_bin_dir.iterdir(), key=lambda path: path.name.lower()):
        if not entry.is_file():
            continue
        if entry.suffix.lower() not in RUNTIME_SIDECAR_EXTS:
            continue
        shutil.copy2(entry, runtime_workspace_dir / entry.name)
        copied_files.append(entry.name)

    distributed_configs = distribute_configs(
        REPO_ROOT / "config",
        REPO_ROOT / "dist" / "config",
        [WINDOWS_TARGET],
    )
    distributed_notices = generate_notices_outputs(
        REPO_ROOT,
        REPO_ROOT / "dist" / "notices",
        [WINDOWS_TARGET],
    )
    replace_path(distributed_configs[WINDOWS_TARGET], runtime_workspace_dir / "config")
    replace_path(distributed_notices[WINDOWS_TARGET], runtime_workspace_dir / "notices")
    print(
        "==> Synced runtime artifacts to "
        f"{runtime_workspace_dir} ({len(copied_files)} files + config + notices)"
    )


def normalize_cmake_build_extra_args(extra_args: list[str] | None) -> list[str]:
    if not extra_args:
        return []

    normalized = list(extra_args)
    while len(normalized) > 1 and normalized[0] == "--" and normalized[1] == "--":
        normalized.pop(0)
    if normalized == ["--"]:
        return []
    return normalized


def build_spec(
    *,
    build_dir: Path,
    generator: str,
    build_type: str,
    compiler: str,
    target: str,
    tidy_enabled: bool,
    extra_args: list[str],
) -> CMakeProjectSpec:
    return CMakeProjectSpec(
        project_dir=PROJECT_DIR,
        build_dir=build_dir,
        source_dir=PROJECT_DIR,
        generator=generator,
        build_type=build_type,
        compiler=compiler,
        target=target,
        cmake_defines=(
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
            f"-DBILLS_ENABLE_TIDY={'ON' if tidy_enabled else 'OFF'}",
            "-DBILLS_CORE_BUILD_SHARED=OFF",
        ),
        build_args=tuple(extra_args),
    )


def build_cache_policy(*, tidy_enabled: bool) -> CachePolicy:
    rebuild_message = "==> Existing CMake cache does not match expected setup. Recreating dist directory."
    return CachePolicy(
        bool_options=(
            BoolOptionExpectation("BILLS_ENABLE_TIDY", tidy_enabled),
            BoolOptionExpectation("BILLS_CORE_BUILD_SHARED", False),
        ),
        option_mismatch_action="recreate",
        skip_configure_when_cache_matches=True,
        reuse_message="==> Using existing CMake configuration.",
        source_mismatch_message=rebuild_message,
        toolchain_mismatch_message=rebuild_message,
        option_mismatch_message=rebuild_message,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Bills Tracer CLI dist helper")
    parser.add_argument(
        "--preset",
        choices=["debug", "release", "tidy"],
        default="debug",
        help="Dist preset to use.",
    )
    parser.add_argument(
        "--scope",
        choices=["shared", "isolated"],
        default="shared",
        help="Dist scope to use. `tidy` only supports shared.",
    )
    parser.add_argument(
        "--instance-id",
        default="",
        help="Optional isolated dist instance id. Defaults to 'manual'.",
    )
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--target", default="bills_tracer_cli")
    parser.add_argument(
        "--compiler",
        choices=["clang"],
        default="clang",
        help="Compiler choice (clang only).",
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Extra args for `cmake --build`.",
    )
    args = parser.parse_args()
    if any(flag in {"--core-shared", "--core-static"} for flag in args.extra):
        print("!!! Error: Windows bills_tracer_cli dist no longer accepts --core-shared or --core-static.")
        return 2

    spec = resolve_build_directory(
        REPO_ROOT,
        target="bills-tracer-cli",
        preset=args.preset,
        scope=args.scope,
        instance_id=args.instance_id,
    )
    extra_args = normalize_cmake_build_extra_args(args.extra)
    tidy_enabled = args.preset == "tidy"
    target_name = "tidy" if tidy_enabled else args.target
    build_result = configure_and_build(
        build_spec(
            build_dir=spec.build_dir,
            generator=args.generator,
            build_type=spec.cmake_build_type,
            compiler=args.compiler,
            target=target_name,
            tidy_enabled=tidy_enabled,
            extra_args=extra_args,
        ),
        build_cache_policy(tidy_enabled=tidy_enabled),
        capture_output=True,
        log_path=spec.build_dir / "build.log",
    )
    if not build_result.success:
        return 1

    if tidy_enabled:
        if build_result.log_lines:
            print(f"==> Legacy task splitting into {spec.build_dir / 'tasks'} is disabled.")
            print(
                "==> Use `python tools/run.py tidy` + "
                "`python tools/run.py tidy-split` to generate the canonical queue "
                "under `temp/tidy/tasks`."
            )
        else:
            print("==> No compile output captured; skipping task split.")
        return 0

    try:
        sync_runtime_artifacts(spec.build_dir)
    except (FileNotFoundError, ValueError, OSError) as exc:
        print(f"!!! Failed to distribute runtime config: {exc}")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
