#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    resolve_build_directory,
    resolve_runtime_workspace_dir,
)
from tools.flows.bills_tracer_flow_support.config_distribution import (
    WINDOWS_TARGET,
    distribute_configs,
)
from tools.notices.notices_support import generate_notices_outputs

PROJECT_DIR = REPO_ROOT / "apps" / "bills_cli"
RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".manifest", ".pdb"}


def run_command(command: list[str], cwd: Path) -> None:
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        print(f"\n!!! A dist preparation step failed with exit code {exc.returncode}.")
        sys.exit(exc.returncode)


def replace_path(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        if dst.is_dir():
            shutil.rmtree(dst)
        else:
            dst.unlink()
    if src.is_dir():
        shutil.copytree(src, dst)
    else:
        shutil.copy2(src, dst)


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


def read_cache_home_directory(cache_file: Path) -> Path | None:
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
                    return Path(line.split("=", 1)[1].strip())
    except OSError:
        return None
    return None


def read_cache_compilers(cache_file: Path) -> tuple[str | None, str | None]:
    c_compiler = None
    cxx_compiler = None
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_C_COMPILER:FILEPATH="):
                    c_compiler = line.split("=", 1)[1].strip()
                elif line.startswith("CMAKE_CXX_COMPILER:FILEPATH="):
                    cxx_compiler = line.split("=", 1)[1].strip()
    except OSError:
        return None, None
    return c_compiler, cxx_compiler


def cache_matches_compiler(cache_file: Path, compiler: str) -> bool:
    if not compiler:
        return True
    c_compiler, cxx_compiler = read_cache_compilers(cache_file)
    if not c_compiler or not cxx_compiler:
        return False
    c_name = Path(c_compiler).name.lower()
    cxx_name = Path(cxx_compiler).name.lower()
    if compiler == "clang":
        return "clang" in c_name and "clang" in cxx_name
    if compiler == "gcc":
        return c_name.startswith("gcc") and (
            cxx_name.startswith("g++") or cxx_name.startswith("c++")
        )
    return False


def read_cache_bool_option(cache_file: Path, key: str) -> bool | None:
    prefix = f"{key}:BOOL="
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith(prefix):
                    value = line.split("=", 1)[1].strip().upper()
                    if value == "ON":
                        return True
                    if value == "OFF":
                        return False
                    return None
    except OSError:
        return None
    return None


def ensure_cmake_configured(
    build_dir: Path,
    *,
    generator: str,
    build_type: str,
    compiler: str,
    tidy_enabled: bool,
    core_shared: bool,
) -> None:
    if not build_dir.exists():
        print(f"==> Creating dist directory: {build_dir}")
        build_dir.mkdir(parents=True)

    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        cached_home = read_cache_home_directory(cache_file)
        cached_tidy_enabled = read_cache_bool_option(cache_file, "BILLS_ENABLE_TIDY")
        cached_core_shared = read_cache_bool_option(cache_file, "BILLS_CORE_BUILD_SHARED")
        if (
            cached_home is not None
            and cached_home.resolve() == PROJECT_DIR.resolve()
            and cache_matches_compiler(cache_file, compiler)
            and cached_tidy_enabled == tidy_enabled
            and cached_core_shared == core_shared
        ):
            print("==> Using existing CMake configuration.")
            return
        print("==> Existing CMake cache does not match expected setup. Recreating dist directory.")
        try:
            shutil.rmtree(build_dir)
        except FileNotFoundError:
            # Windows/Ninja can leave the tree in a partially removed state between runs.
            pass
        build_dir.mkdir(parents=True, exist_ok=True)

    cmake_args = [
        "cmake",
        "-S",
        str(PROJECT_DIR),
        "-B",
        str(build_dir),
        "-G",
        generator,
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
        "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
    ]
    if compiler:
        cmake_args.append(f"-DBILL_COMPILER={compiler}")
    cmake_args.append(f"-DBILLS_ENABLE_TIDY={'ON' if tidy_enabled else 'OFF'}")
    cmake_args.append(f"-DBILLS_CORE_BUILD_SHARED={'ON' if core_shared else 'OFF'}")
    run_command(cmake_args, cwd=PROJECT_DIR)


def run_build_capture(
    build_dir: Path,
    *,
    target: str | None,
    extra_args: list[str] | None,
) -> tuple[list[str], bool]:
    command = ["cmake", "--build", str(build_dir)]
    if target:
        command.extend(["--target", target])
    if extra_args:
        command.extend(extra_args)

    print(f"==> Running command: {' '.join(command)}")
    try:
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        sys.exit(1)

    log_path = build_dir / "build.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)

    log_lines: list[str] = []
    assert process.stdout is not None
    with log_path.open("w", encoding="utf-8") as log_file:
        for line in process.stdout:
            log_lines.append(line)
            log_file.write(line)
            print(line, end="")

    process.wait()
    success = process.returncode == 0
    if success:
        print("==> Dist preparation finished successfully.")
    else:
        print(f"==> Dist preparation failed with exit code {process.returncode}.")
    return log_lines, success


def normalize_cmake_build_extra_args(extra_args: list[str] | None) -> list[str]:
    if not extra_args:
        return []

    normalized = list(extra_args)
    while len(normalized) > 1 and normalized[0] == "--" and normalized[1] == "--":
        normalized.pop(0)
    if normalized == ["--"]:
        return []
    return normalized


def main() -> int:
    parser = argparse.ArgumentParser(description="Bills Master dist helper")
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
    parser.add_argument("--target", default="bill_master_cli")
    parser.add_argument(
        "--compiler",
        choices=["clang", "gcc"],
        default="clang",
        help="Compiler choice.",
    )
    parser.add_argument(
        "--core-shared",
        dest="core_shared",
        action="store_true",
        default=False,
        help="Emit bills_core as shared library into dist.",
    )
    parser.add_argument(
        "--core-static",
        dest="core_shared",
        action="store_false",
        help="Emit bills_core as static library into dist (default).",
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Extra args for `cmake --build`.",
    )
    args = parser.parse_args()

    spec = resolve_build_directory(
        REPO_ROOT,
        target="bills",
        preset=args.preset,
        scope=args.scope,
        instance_id=args.instance_id,
    )
    extra_args = normalize_cmake_build_extra_args(args.extra)
    tidy_enabled = args.preset == "tidy"
    target_name = "tidy" if tidy_enabled else args.target

    ensure_cmake_configured(
        spec.build_dir,
        generator=args.generator,
        build_type=spec.cmake_build_type,
        compiler=args.compiler,
        tidy_enabled=tidy_enabled,
        core_shared=args.core_shared,
    )

    log_lines, success = run_build_capture(
        spec.build_dir,
        target=target_name,
        extra_args=extra_args,
    )
    if not success:
        return 1

    if tidy_enabled:
        if log_lines:
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
