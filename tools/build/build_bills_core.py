#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
PROJECT_DIR = REPO_ROOT / "libs" / "bills_core"


def run_command(command: list[str], cwd: Path) -> None:
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(
            f"!!! Error: Command '{command[0]}' not found. "
            "Is it installed and in your PATH?"
        )
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        print(f"\n!!! A build step failed with exit code {exc.returncode}.")
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


def read_cache_cxx_compiler(cache_file: Path) -> str | None:
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_CXX_COMPILER:FILEPATH="):
                    return line.split("=", 1)[1].strip()
    except OSError:
        return None
    return None


def cache_matches_compiler(cache_file: Path, compiler: str) -> bool:
    cached = read_cache_cxx_compiler(cache_file)
    if not cached:
        return False
    name = Path(cached).name.lower()
    if compiler == "clang":
        return "clang" in name
    if compiler == "gcc":
        return name.startswith("g++") or name.startswith("c++")
    return False


def ensure_cmake_configured(
    build_dir: Path, generator: str, build_type: str, shared: bool, compiler: str
) -> None:
    if not build_dir.exists():
        print(f"==> Creating build directory: {build_dir}")
        build_dir.mkdir(parents=True)

    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        cached_home = read_cache_home_directory(cache_file)
        if cached_home is not None and cached_home.resolve() == PROJECT_DIR.resolve():
            if cache_matches_compiler(cache_file, compiler):
                print("==> Refreshing existing CMake configuration.")
            else:
                print("==> Existing CMake cache compiler mismatch. Recreating build directory.")
                shutil.rmtree(build_dir)
                build_dir.mkdir(parents=True, exist_ok=True)
        else:
            print("==> Existing CMake cache points to a different source. Recreating build directory.")
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
    build_dir: Path, build_type: str, shared: bool, compiler: str,
    extra_args: list[str] | None = None
) -> None:
    ensure_cmake_configured(
        build_dir=build_dir,
        generator="Ninja",
        build_type=build_type,
        shared=shared,
        compiler=compiler,
    )

    command = ["cmake", "--build", str(build_dir), "--target", "bills_core"]
    if extra_args:
        command.extend(extra_args)
    run_command(command, cwd=PROJECT_DIR)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build helper for libs/bills_core")
    parser.add_argument(
        "command",
        choices=["build", "build_fast"],
        help="build=Release, build_fast=Debug",
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
        help="Build shared library (default)",
    )
    parser.add_argument(
        "--static",
        dest="shared",
        action="store_false",
        help="Build static library",
    )

    args, extra_args = parser.parse_known_args()
    if extra_args and extra_args[0] == "--":
        extra_args = extra_args[1:]

    if args.command == "build":
        run_build(
            PROJECT_DIR / "build",
            build_type="Release",
            shared=args.shared,
            compiler=args.compiler,
            extra_args=extra_args,
        )
    elif args.command == "build_fast":
        run_build(
            PROJECT_DIR / "build_fast",
            build_type="Debug",
            shared=args.shared,
            compiler=args.compiler,
            extra_args=extra_args,
        )
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
