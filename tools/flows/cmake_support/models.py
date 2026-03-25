from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Literal

CacheMismatchAction = Literal["reconfigure", "recreate"]


@dataclass(frozen=True)
class BoolOptionExpectation:
    key: str
    expected: bool


@dataclass(frozen=True)
class CachePolicy:
    require_matching_source: bool = True
    require_matching_compiler: bool = True
    require_matching_binutils: bool = True
    bool_options: tuple[BoolOptionExpectation, ...] = ()
    option_mismatch_action: CacheMismatchAction = "recreate"
    skip_configure_when_cache_matches: bool = True
    reuse_message: str = "==> Using existing CMake configuration."
    source_mismatch_message: str = (
        "==> Existing CMake cache points to a different source. Recreating dist directory."
    )
    toolchain_mismatch_message: str = (
        "==> Existing CMake cache toolchain selection changed. Recreating dist directory."
    )
    option_mismatch_message: str = (
        "==> Existing CMake cache option mismatch. Reconfiguring build directory in place."
    )


@dataclass(frozen=True)
class CMakeProjectSpec:
    project_dir: Path
    build_dir: Path
    source_dir: Path
    generator: str
    build_type: str
    compiler: str = "clang"
    target: str | None = None
    cmake_defines: tuple[str, ...] = ()
    build_args: tuple[str, ...] = ()


@dataclass(frozen=True)
class BuildResult:
    log_lines: list[str]
    success: bool
