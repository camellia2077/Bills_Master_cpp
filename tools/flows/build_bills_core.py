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

from tools.toolchain.services.build_layout import resolve_build_directory

PROJECT_DIR = REPO_ROOT / "libs" / "bills_core"


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
    shared: bool,
    compiler: str,
    modules_enabled: bool,
) -> None:
    if not build_dir.exists():
        print(f"==> Creating dist directory: {build_dir}")
        build_dir.mkdir(parents=True)

    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        cached_home = read_cache_home_directory(cache_file)
        cached_modules_enabled = read_cache_bool_option(cache_file, "BILLS_ENABLE_MODULES")
        if cached_home is not None and cached_home.resolve() == PROJECT_DIR.resolve():
            if (
                cache_matches_compiler(cache_file, compiler)
                and cached_modules_enabled == modules_enabled
            ):
                print("==> Refreshing existing CMake configuration.")
            else:
                print(
                    "==> Existing CMake cache compiler/module options mismatch. "
                    "Recreating dist directory."
                )
                shutil.rmtree(build_dir)
                build_dir.mkdir(parents=True, exist_ok=True)
        else:
            print(
                "==> Existing CMake cache points to a different source. Recreating dist directory."
            )
            shutil.rmtree(build_dir)
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
        f"-DBILLS_CORE_BUILD_SHARED={'ON' if shared else 'OFF'}",
        f"-DBILLS_ENABLE_MODULES={'ON' if modules_enabled else 'OFF'}",
    ]
    if compiler:
        compiler_value = compiler.strip().lower()
        if compiler_value == "clang":
            cmake_args.append("-DCMAKE_CXX_COMPILER=clang++")
        elif compiler_value == "gcc":
            cmake_args.append("-DCMAKE_CXX_COMPILER=g++")
        else:
            print("!!! Error: compiler must be 'clang' or 'gcc'.")
            sys.exit(1)
    run_command(cmake_args, cwd=PROJECT_DIR)


def run_build(
    build_dir: Path,
    *,
    build_type: str,
    shared: bool,
    compiler: str,
    modules_enabled: bool,
    extra_args: list[str] | None,
) -> None:
    ensure_cmake_configured(
        build_dir=build_dir,
        generator="Ninja",
        build_type=build_type,
        shared=shared,
        compiler=compiler,
        modules_enabled=modules_enabled,
    )

    command = ["cmake", "--build", str(build_dir), "--target", "bills_core"]
    if extra_args:
        command.extend(extra_args)
    run_command(command, cwd=PROJECT_DIR)


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
        choices=["clang", "gcc"],
        default="clang",
        help="Compiler choice (default: clang)",
    )
    parser.add_argument(
        "--shared",
        dest="shared",
        action="store_true",
        default=True,
        help="Emit shared library into dist (default).",
    )
    parser.add_argument(
        "--static",
        dest="shared",
        action="store_false",
        help="Emit static library into dist.",
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

    spec = resolve_build_directory(
        REPO_ROOT,
        target="core",
        preset=args.preset,
        scope="shared",
    )
    run_build(
        spec.build_dir,
        build_type=spec.cmake_build_type,
        shared=args.shared,
        compiler=args.compiler,
        modules_enabled=args.modules_enabled,
        extra_args=extra_args,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
