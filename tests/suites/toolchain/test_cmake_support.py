from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.flows.cmake_support.models import (
    BoolOptionExpectation,
    BuildResult,
    CachePolicy,
    CMakeProjectSpec,
)
from tools.flows.cmake_support.runner import configure_and_build, ensure_configured


def write_cache(
    build_dir: Path,
    *,
    home: Path,
    c_compiler: str = "C:/msys64/mingw64/bin/clang.exe",
    cxx_compiler: str = "C:/msys64/mingw64/bin/clang++.exe",
    ar: str = "C:/msys64/mingw64/bin/ar.exe",
    ranlib: str = "C:/msys64/mingw64/bin/ranlib.exe",
    make_program: str = "C:/msys64/mingw64/bin/ninja.exe",
    bool_options: dict[str, bool] | None = None,
) -> Path:
    cache_file = build_dir / "CMakeCache.txt"
    lines = [
        f"CMAKE_HOME_DIRECTORY:INTERNAL={home}",
        f"CMAKE_C_COMPILER:FILEPATH={c_compiler}",
        f"CMAKE_CXX_COMPILER:FILEPATH={cxx_compiler}",
        f"CMAKE_AR:FILEPATH={ar}",
        f"CMAKE_RANLIB:FILEPATH={ranlib}",
        f"CMAKE_MAKE_PROGRAM:FILEPATH={make_program}",
    ]
    for key, value in (bool_options or {}).items():
        lines.append(f"{key}:BOOL={'ON' if value else 'OFF'}")
    cache_file.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return cache_file


class CMakeSupportTests(unittest.TestCase):
    def make_spec(self, *, root: Path, build_dir: Path, source_dir: Path, compiler: str = "clang") -> CMakeProjectSpec:
        return CMakeProjectSpec(
            project_dir=root,
            build_dir=build_dir,
            source_dir=source_dir,
            generator="Ninja",
            build_type="Debug",
            compiler=compiler,
            target="demo",
            cmake_defines=("-DDEMO=ON",),
        )

    def test_source_mismatch_recreates_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            build_dir.mkdir()
            source_dir = root / "src"
            source_dir.mkdir()
            write_cache(build_dir, home=root / "other")
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)
            policy = CachePolicy()

            with (
                patch("tools.flows.cmake_support.runner.recreate_build_dir") as recreate_mock,
                patch("tools.flows.cmake_support.runner.run_command") as run_command_mock,
            ):
                ensure_configured(spec, policy)

        recreate_mock.assert_called_once_with(build_dir)
        run_command_mock.assert_called_once()

    def test_existing_build_dir_without_cache_recreates_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            build_dir.mkdir()
            (build_dir / "_deps").mkdir()
            source_dir = root / "src"
            source_dir.mkdir()
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)

            with (
                patch("tools.flows.cmake_support.runner.recreate_build_dir") as recreate_mock,
                patch("tools.flows.cmake_support.runner.run_command") as run_command_mock,
            ):
                ensure_configured(spec, CachePolicy())

        recreate_mock.assert_called_once_with(build_dir)
        run_command_mock.assert_called_once()

    def test_clang_only_validation_rejects_non_clang(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            source_dir = root / "src"
            source_dir.mkdir()
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir, compiler="gcc")

            with self.assertRaises(SystemExit) as exc:
                ensure_configured(spec, CachePolicy())

        self.assertEqual(exc.exception.code, 1)

    def test_windows_binutils_mismatch_recreates_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            build_dir.mkdir()
            source_dir = root / "src"
            source_dir.mkdir()
            write_cache(
                build_dir,
                home=source_dir,
                make_program="C:/msys64/ucrt64/bin/ninja.exe",
            )
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)

            with (
                patch("tools.flows.cmake_support.runner.recreate_build_dir") as recreate_mock,
                patch("tools.flows.cmake_support.runner.run_command") as run_command_mock,
            ):
                with patch("tools.flows.cmake_support.cache.sys.platform", "win32"):
                    ensure_configured(spec, CachePolicy())

        recreate_mock.assert_called_once_with(build_dir)
        run_command_mock.assert_called_once()

    def test_option_mismatch_recreate_policy_recreates_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            build_dir.mkdir()
            source_dir = root / "src"
            source_dir.mkdir()
            write_cache(
                build_dir,
                home=source_dir,
                bool_options={"BILLS_ENABLE_TIDY": False},
            )
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)
            policy = CachePolicy(
                bool_options=(BoolOptionExpectation("BILLS_ENABLE_TIDY", True),),
                option_mismatch_action="recreate",
            )

            with (
                patch("tools.flows.cmake_support.runner.recreate_build_dir") as recreate_mock,
                patch("tools.flows.cmake_support.runner.run_command") as run_command_mock,
            ):
                ensure_configured(spec, policy)

        recreate_mock.assert_called_once_with(build_dir)
        run_command_mock.assert_called_once()

    def test_option_mismatch_reconfigure_policy_skips_recreate(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            build_dir.mkdir()
            source_dir = root / "src"
            source_dir.mkdir()
            write_cache(
                build_dir,
                home=source_dir,
                bool_options={"BILLS_ENABLE_MODULES": False},
            )
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)
            policy = CachePolicy(
                bool_options=(BoolOptionExpectation("BILLS_ENABLE_MODULES", True),),
                option_mismatch_action="reconfigure",
            )

            with (
                patch("tools.flows.cmake_support.runner.recreate_build_dir") as recreate_mock,
                patch("tools.flows.cmake_support.runner.run_command") as run_command_mock,
            ):
                ensure_configured(spec, policy)

        recreate_mock.assert_not_called()
        run_command_mock.assert_called_once()

    def test_configure_and_build_capture_returns_logs_and_success(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            build_dir = root / "build"
            source_dir = root / "src"
            source_dir.mkdir()
            spec = self.make_spec(root=root, build_dir=build_dir, source_dir=source_dir)
            expected = BuildResult(log_lines=["line\n"], success=True)

            with (
                patch("tools.flows.cmake_support.runner.ensure_configured") as ensure_mock,
                patch(
                    "tools.flows.cmake_support.runner.run_build_capture",
                    return_value=expected,
                ) as capture_mock,
            ):
                result = configure_and_build(
                    spec,
                    CachePolicy(),
                    capture_output=True,
                    log_path=build_dir / "build.log",
                )

        ensure_mock.assert_called_once()
        capture_mock.assert_called_once()
        self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
