from __future__ import annotations

from pathlib import Path

from .cache import (
    cache_matches_compiler,
    cache_matches_source,
    cache_matches_windows_binutils,
    find_bool_option_mismatches,
)
from .filesystem import recreate_build_dir
from .models import BuildResult, CachePolicy, CMakeProjectSpec
from .process import run_build_capture, run_command


def _normalize_compiler(compiler: str) -> str:
    compiler_value = compiler.strip().lower() if compiler else "clang"
    if compiler_value != "clang":
        print("!!! Error: compiler must be 'clang'. GCC is no longer supported.")
        raise SystemExit(1)
    return compiler_value


def ensure_configured(spec: CMakeProjectSpec, policy: CachePolicy) -> None:
    _normalize_compiler(spec.compiler)

    if not spec.build_dir.exists():
        print(f"==> Creating dist directory: {spec.build_dir}")
        spec.build_dir.mkdir(parents=True)

    cache_file = spec.build_dir / "CMakeCache.txt"
    should_run_configure = not cache_file.is_file()

    if spec.build_dir.exists() and not cache_file.is_file() and any(spec.build_dir.iterdir()):
        print("==> Existing dist directory has no usable CMake cache. Recreating dist directory.")
        recreate_build_dir(spec.build_dir)
        should_run_configure = True

    if cache_file.is_file():
        if policy.require_matching_source and not cache_matches_source(cache_file, spec.source_dir):
            print(policy.source_mismatch_message)
            recreate_build_dir(spec.build_dir)
            should_run_configure = True
        elif (
            (policy.require_matching_compiler and not cache_matches_compiler(cache_file, spec.compiler))
            or (
                policy.require_matching_binutils
                and not cache_matches_windows_binutils(cache_file)
            )
        ):
            print(policy.toolchain_mismatch_message)
            recreate_build_dir(spec.build_dir)
            should_run_configure = True
        else:
            option_mismatches = find_bool_option_mismatches(cache_file, policy.bool_options)
            if option_mismatches:
                print(policy.option_mismatch_message)
                if policy.option_mismatch_action == "recreate":
                    recreate_build_dir(spec.build_dir)
                should_run_configure = True
            else:
                print(policy.reuse_message)
                should_run_configure = not policy.skip_configure_when_cache_matches

    if not should_run_configure:
        return

    cmake_args = [
        "cmake",
        "-S",
        str(spec.source_dir),
        "-B",
        str(spec.build_dir),
        "-G",
        spec.generator,
        f"-DCMAKE_BUILD_TYPE={spec.build_type}",
        "-DBILL_COMPILER=clang",
        *spec.cmake_defines,
    ]
    run_command(cmake_args, cwd=spec.project_dir)


def build_target(
    spec: CMakeProjectSpec,
    *,
    capture_output: bool = False,
    log_path: Path | None = None,
    success_message: str = "==> Dist preparation finished successfully.",
    failure_prefix: str = "==> Dist preparation failed with exit code ",
) -> BuildResult:
    command = ["cmake", "--build", str(spec.build_dir)]
    if spec.target:
        command.extend(["--target", spec.target])
    if spec.build_args:
        command.extend(spec.build_args)

    if capture_output:
        return run_build_capture(
            command,
            cwd=spec.project_dir,
            log_path=log_path or (spec.build_dir / "build.log"),
            success_message=success_message,
            failure_prefix=failure_prefix,
        )

    run_command(command, cwd=spec.project_dir)
    return BuildResult(log_lines=[], success=True)


def configure_and_build(
    spec: CMakeProjectSpec,
    policy: CachePolicy,
    *,
    capture_output: bool = False,
    log_path: Path | None = None,
    success_message: str = "==> Dist preparation finished successfully.",
    failure_prefix: str = "==> Dist preparation failed with exit code ",
) -> BuildResult:
    ensure_configured(spec, policy)
    return build_target(
        spec,
        capture_output=capture_output,
        log_path=log_path,
        success_message=success_message,
        failure_prefix=failure_prefix,
    )
