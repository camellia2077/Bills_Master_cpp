from __future__ import annotations

import sys
from pathlib import Path

from .models import BoolOptionExpectation


def read_cache_value(cache_file: Path, prefixes: list[str]) -> str:
    try:
        cache_text = cache_file.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ""
    for line in cache_text.splitlines():
        for prefix in prefixes:
            if line.startswith(prefix):
                return line.split("=", 1)[1].strip()
    return ""


def read_cache_home_directory(cache_file: Path) -> Path | None:
    value = read_cache_value(cache_file, ["CMAKE_HOME_DIRECTORY:INTERNAL="])
    return Path(value) if value else None


def read_cache_compilers(cache_file: Path) -> tuple[str | None, str | None]:
    c_compiler = read_cache_value(
        cache_file,
        ["CMAKE_C_COMPILER:FILEPATH=", "CMAKE_C_COMPILER:STRING="],
    )
    cxx_compiler = read_cache_value(
        cache_file,
        ["CMAKE_CXX_COMPILER:FILEPATH=", "CMAKE_CXX_COMPILER:STRING="],
    )
    return (c_compiler or None, cxx_compiler or None)


def read_cache_tool_path(cache_file: Path, key: str) -> str | None:
    value = read_cache_value(cache_file, [f"{key}:FILEPATH=", f"{key}:STRING="])
    return value or None


def read_cache_bool_option(cache_file: Path, key: str) -> bool | None:
    value = read_cache_value(cache_file, [f"{key}:BOOL="]).upper()
    if value == "ON":
        return True
    if value == "OFF":
        return False
    return None


def cache_matches_source(cache_file: Path, source_dir: Path) -> bool:
    cached_home = read_cache_home_directory(cache_file)
    return cached_home is not None and cached_home.resolve() == source_dir.resolve()


def cache_matches_compiler(cache_file: Path, compiler: str) -> bool:
    compiler_value = compiler.strip().lower() if compiler else "clang"
    if compiler_value != "clang":
        return False
    c_compiler, cxx_compiler = read_cache_compilers(cache_file)
    if not c_compiler or not cxx_compiler:
        return False
    c_name = Path(c_compiler).name.lower()
    cxx_name = Path(cxx_compiler).name.lower()
    return "clang" in c_name and "clang" in cxx_name


def cache_matches_windows_binutils(cache_file: Path, *, platform_name: str | None = None) -> bool:
    active_platform = platform_name or sys.platform
    if active_platform != "win32":
        return True
    cached_ar = (read_cache_tool_path(cache_file, "CMAKE_AR") or "").replace("\\", "/").lower()
    cached_ranlib = (read_cache_tool_path(cache_file, "CMAKE_RANLIB") or "").replace(
        "\\", "/"
    ).lower()
    cached_make_program = (
        read_cache_tool_path(cache_file, "CMAKE_MAKE_PROGRAM") or ""
    ).replace("\\", "/").lower()
    return (
        cached_ar.endswith("/mingw64/bin/ar.exe")
        and cached_ranlib.endswith("/mingw64/bin/ranlib.exe")
        and cached_make_program.endswith("/mingw64/bin/ninja.exe")
    )


def find_bool_option_mismatches(
    cache_file: Path,
    expectations: tuple[BoolOptionExpectation, ...],
) -> list[BoolOptionExpectation]:
    mismatches: list[BoolOptionExpectation] = []
    for expectation in expectations:
        actual = read_cache_bool_option(cache_file, expectation.key)
        if actual != expectation.expected:
            mismatches.append(expectation)
    return mismatches
