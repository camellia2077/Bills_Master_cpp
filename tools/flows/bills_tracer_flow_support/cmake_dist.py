from __future__ import annotations

from pathlib import Path

from tools.flows.cmake_support.models import CachePolicy, CMakeProjectSpec
from tools.flows.cmake_support.runner import configure_and_build
from tools.toolchain.services.build_layout import (
    resolve_build_directory,
    sanitize_segment,
    short_hash,
)

from .locks import directory_lock


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
        target="bills-tracer-cli",
        preset=build_preset,
        scope=build_scope,
        instance_id=instance_id,
    ).build_dir


def build_spec(
    *,
    source_dir: Path,
    build_dir: Path,
    generator: str,
    build_type: str,
    target: str,
    cmake_defines: list[str],
) -> CMakeProjectSpec:
    return CMakeProjectSpec(
        project_dir=source_dir,
        build_dir=build_dir,
        source_dir=source_dir,
        generator=generator,
        build_type=build_type,
        compiler="clang",
        target=target,
        cmake_defines=(
            "-DBILLS_CORE_BUILD_SHARED=OFF",
            *cmake_defines,
        ),
    )


def build_cache_policy() -> CachePolicy:
    return CachePolicy(
        skip_configure_when_cache_matches=False,
        reuse_message="==> Refreshing existing CMake configuration.",
        source_mismatch_message=(
            "==> Existing CMake cache points to a different source. Recreating dist directory."
        ),
        toolchain_mismatch_message=(
            "==> Existing CMake cache toolchain selection changed. Recreating dist directory."
        ),
    )


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
        configure_and_build(
            build_spec(
                source_dir=source_dir,
                build_dir=build_dir,
                generator=generator,
                build_type=build_type,
                target=target,
                cmake_defines=cmake_defines,
            ),
            build_cache_policy(),
        )
