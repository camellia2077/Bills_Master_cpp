from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from tools.toolchain.services.build_layout import (
    resolve_build_directory,
    sanitize_segment,
    short_hash,
)

from .locks import directory_lock


def run_command(
    command: list[str], cwd: Path | None = None, env: dict[str, str] | None = None
) -> None:
    print(f"==> {' '.join(command)}")
    subprocess.run(command, cwd=str(cwd) if cwd else None, env=env, check=True)


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


def cache_uses_clang(cache_file: Path) -> bool:
    c_compiler, cxx_compiler = read_cache_compilers(cache_file)
    if not c_compiler or not cxx_compiler:
        return False
    c_name = Path(c_compiler).name.lower()
    cxx_name = Path(cxx_compiler).name.lower()
    return "clang" in c_name and "clang" in cxx_name


def prepare_build_dir(build_dir: Path, source_dir: Path) -> None:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return
    cached_home = read_cache_home_directory(cache_file)
    if (
        cached_home is not None
        and cached_home.resolve() == source_dir.resolve()
        and cache_uses_clang(cache_file)
    ):
        return
    shutil.rmtree(build_dir)


def resolve_build_dir(
    repo_root: Path,
    build_scope: str,
    build_preset: str,
    output_project: str,
    export_pipeline: str,
    formats: list[str],
    generator: str,
) -> Path:
    instance_id = ""
    if build_scope == "isolated":
        format_tag = sanitize_segment("-".join(formats))
        project_tag = sanitize_segment(output_project)
        pipeline_tag = sanitize_segment(export_pipeline)
        generator_tag = sanitize_segment(generator.lower())
        unique_seed = f"{project_tag}|{pipeline_tag}|{format_tag}|{build_preset}|{generator_tag}"
        instance_id = f"b_{short_hash(unique_seed, length=12)}"
    return resolve_build_directory(
        repo_root,
        target="bills",
        preset=build_preset,
        scope=build_scope,
        instance_id=instance_id,
    ).build_dir


def build_cli(
    source_dir: Path,
    build_dir: Path,
    generator: str,
    build_type: str,
    target: str,
    cmake_defines: list[str],
) -> None:
    lock_dir = build_dir.parent / f"{build_dir.name}.lock"
    with directory_lock(lock_dir):
        prepare_build_dir(build_dir, source_dir)
        configure_cmd = [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(build_dir),
            "-G",
            generator,
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DBILL_COMPILER=clang",
            "-DBILLS_CORE_BUILD_SHARED=ON",
            *cmake_defines,
        ]
        run_command(configure_cmd)

        build_cmd = ["cmake", "--build", str(build_dir), "--target", target]
        run_command(build_cmd)
