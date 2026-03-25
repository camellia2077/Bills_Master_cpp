import sys
from pathlib import Path

from tools.flows.cmake_support.models import CachePolicy, CMakeProjectSpec
from tools.flows.cmake_support.runner import build_target as run_build_target
from tools.flows.cmake_support.runner import ensure_configured as run_ensure_configured


def build_spec(
    *,
    project_dir: Path,
    build_dir: Path,
    build_type: str,
    generator: str,
    source_dir: str,
    compiler: str,
    target: str | None = None,
) -> CMakeProjectSpec:
    src_path = Path(source_dir).resolve()
    return CMakeProjectSpec(
        project_dir=project_dir,
        build_dir=build_dir,
        source_dir=src_path,
        generator=generator,
        build_type=build_type,
        compiler=compiler,
        target=target,
        cmake_defines=(
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
        ),
    )


def build_cache_policy(build_dir: Path) -> CachePolicy:
    return CachePolicy(
        option_mismatch_action="recreate",
        skip_configure_when_cache_matches=True,
        reuse_message=f"==> Reusing existing dist directory: {build_dir.resolve()}",
        source_mismatch_message=(
            "==> Existing CMake cache points to another source directory. "
            "Cleaning dist directory for reconfigure..."
        ),
        toolchain_mismatch_message=(
            "==> Existing CMake cache toolchain selection changed. "
            "Cleaning dist directory for reconfigure..."
        ),
        option_mismatch_message=(
            "==> Existing CMake cache option mismatch. "
            "Cleaning dist directory for reconfigure..."
        ),
    )


def ensure_cmake_configured(
    build_dir: Path,
    build_type: str,
    generator: str,
    source_dir: str,
    compiler: str,
) -> CMakeProjectSpec:
    src_path = Path(source_dir).resolve()
    if not src_path.is_dir():
        print(
            "!!! Error: The CMake source directory specified in config "
            f"does not exist: {source_dir}"
        )
        sys.exit(1)

    spec = build_spec(
        project_dir=src_path,
        build_dir=build_dir,
        build_type=build_type,
        generator=generator,
        source_dir=str(src_path),
        compiler=compiler,
    )
    run_ensure_configured(spec, build_cache_policy(build_dir))
    return spec


def build_target(spec: CMakeProjectSpec) -> None:
    run_build_target(spec)
