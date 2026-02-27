#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

from bills_master_builder.task_splitter import split_tidy_logs

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
PROJECT_DIR = REPO_ROOT / "apps" / "bills_windows_cli"


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
    build_dir: Path, generator: str, build_type: str, compiler: str,
    tidy_enabled: bool = True
) -> None:
    if not build_dir.exists():
        print(f"==> Creating build directory: {build_dir}")
        build_dir.mkdir(parents=True)

    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        cached_home = read_cache_home_directory(cache_file)
        cached_tidy_enabled = read_cache_bool_option(cache_file, "BILLS_ENABLE_TIDY")
        if (
            cached_home is not None
            and cached_home.resolve() == PROJECT_DIR.resolve()
            and cache_matches_compiler(cache_file, compiler)
            and cached_tidy_enabled == tidy_enabled
        ):
            print("==> Using existing CMake configuration.")
            return
        print("==> Existing CMake cache does not match source/compiler. Recreating build directory.")
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
        "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
        "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
    ]
    if compiler:
        cmake_args.append(f"-DBILL_COMPILER={compiler}")
    cmake_args.append(f"-DBILLS_ENABLE_TIDY={'ON' if tidy_enabled else 'OFF'}")

    run_command(cmake_args, cwd=PROJECT_DIR)


def run_build_capture(
    build_dir: Path, target: str | None, extra_args: list[str] | None = None
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
        print(
            f"!!! Error: Command '{command[0]}' not found. "
            "Is it installed and in your PATH?"
        )
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
        print("==> Build finished successfully.")
    else:
        print(f"==> Build failed with exit code {process.returncode}.")
    return log_lines, success


def run_build(
    build_dir: Path,
    build_type: str,
    extra_args: list[str] | None = None,
    tidy_enabled: bool = True,
) -> list[str]:
    ensure_cmake_configured(
        build_dir=build_dir,
        generator="Ninja",
        build_type=build_type,
        compiler="clang",
        tidy_enabled=tidy_enabled,
    )

    log_lines, success = run_build_capture(
        build_dir, target=None, extra_args=extra_args
    )
    if not success:
        sys.exit(1)
    return log_lines


def run_tidy(extra_args: list[str] | None = None) -> None:
    build_dir = PROJECT_DIR / "build_tidy"

    ensure_cmake_configured(
        build_dir=build_dir,
        generator="Ninja",
        build_type="Debug",
        compiler="clang",
        tidy_enabled=True,
    )

    print("==> Running clang-tidy checks...")
    log_lines, success = run_build_capture(
        build_dir, target="tidy", extra_args=extra_args
    )

    if log_lines:
        tasks_dir = build_dir / "tasks"
        count = split_tidy_logs(log_lines, tasks_dir)
        print(f"==> Created {count} task log files in {tasks_dir}")
    else:
        print("==> No build output captured; skipping task split.")

    if not success:
        sys.exit(1)


def main() -> int:
    parser = argparse.ArgumentParser(description="Bills Master build helper")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Run a Release build")
    build_parser.add_argument(
        "extra", nargs=argparse.REMAINDER, help="Extra args for cmake --build"
    )

    build_fast_parser = subparsers.add_parser(
        "build_fast", help="Run a fast Debug build"
    )
    build_fast_parser.add_argument(
        "extra", nargs=argparse.REMAINDER, help="Extra args for cmake --build"
    )

    tidy_parser = subparsers.add_parser("tidy", help="Run clang-tidy checks")
    tidy_parser.add_argument(
        "extra", nargs=argparse.REMAINDER, help="Extra args for cmake --build"
    )

    args = parser.parse_args()

    extra_args: list[str] = []
    if hasattr(args, "extra"):
        extra_args = args.extra
        if extra_args and extra_args[0] == "--":
            extra_args = extra_args[1:]

    if args.command == "build":
        run_build(
            PROJECT_DIR / "build",
            build_type="Release",
            extra_args=extra_args,
            tidy_enabled=True,
        )
    elif args.command == "build_fast":
        run_build(
            PROJECT_DIR / "build_fast",
            build_type="Debug",
            extra_args=extra_args,
            tidy_enabled=False,
        )
    elif args.command == "tidy":
        run_tidy(extra_args=extra_args)
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
