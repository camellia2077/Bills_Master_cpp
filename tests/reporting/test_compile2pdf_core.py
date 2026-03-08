from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools.reporting.compile2pdf.internal import core as facade


class _FakeFuture:
    def __init__(self, result):
        self._result = result

    def result(self):
        return self._result


class _FakeExecutor:
    def __init__(self, *args, **kwargs):
        del args, kwargs

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        del exc_type, exc, tb
        return False

    def submit(self, fn, *args):
        return _FakeFuture(fn(*args))


class Compile2PdfCoreTests(unittest.TestCase):
    def test_core_facade_preserves_public_imports(self) -> None:
        self.assertTrue(callable(facade.compile_single_file))
        self.assertTrue(callable(facade.compile_md_via_typ))
        self.assertTrue(callable(facade.process_directory))
        self.assertTrue(callable(facade.process_directory_md_via_typ))

    def test_compile_single_file_maps_successful_result(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir) / "out"
            input_path = Path(temp_dir) / "demo.md"
            input_path.write_text("demo", encoding="utf-8")

            with patch(
                "tools.reporting.compile2pdf.internal.core_support.single_file.subprocess.run",
                return_value=SimpleNamespace(returncode=0, stdout="", stderr=""),
            ):
                result = facade.compile_single_file(
                    str(input_path),
                    str(output_dir / "demo.pdf"),
                    str(output_dir),
                    lambda *_: ["demo"],
                    "Markdown",
                )

        self.assertTrue(result["success"])
        self.assertEqual(result["file"], "demo.md")

    def test_process_directory_uses_incremental_filter_and_returns_updated_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            source_dir = Path(temp_dir) / "src"
            source_dir.mkdir(parents=True, exist_ok=True)
            output_dir = Path(temp_dir) / "out"

            with patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.discover_tasks",
                return_value=[("a.md", "a.pdf", "dir")],
            ), patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.filter_incremental_tasks",
                return_value=([("a.md", "a.pdf", "dir")], 0, ["a.md"]),
            ), patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.compile_single_file",
                return_value={"success": True, "log": "ok", "duration": 0.1},
            ), patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.concurrent.futures.ProcessPoolExecutor",
                _FakeExecutor,
            ), patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.concurrent.futures.as_completed",
                side_effect=lambda items: list(items),
            ):
                success, failure, duration, updated_files = facade.process_directory(
                    str(source_dir),
                    str(output_dir),
                    ".md",
                    "Markdown",
                    lambda *_: ["demo"],
                    quiet=True,
                )

        self.assertEqual((success, failure), (1, 0))
        self.assertGreaterEqual(duration, 0.0)
        self.assertEqual(updated_files, ["a.md"])

    def test_process_directory_md_via_typ_returns_skipped_results(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            source_dir = Path(temp_dir) / "src"
            source_dir.mkdir(parents=True, exist_ok=True)
            output_dir = Path(temp_dir) / "out"

            with patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.discover_tasks",
                return_value=[("a.md", "a.pdf", "dir")],
            ), patch(
                "tools.reporting.compile2pdf.internal.core_support.directory.filter_incremental_tasks",
                return_value=([], 1, ["a.md"]),
            ):
                results, duration, updated_files = facade.process_directory_md_via_typ(
                    str(source_dir),
                    str(output_dir),
                    font="Noto Serif SC",
                    quiet=True,
                )

        self.assertEqual(results, [{"success": True, "skipped": True}])
        self.assertGreaterEqual(duration, 0.0)
        self.assertEqual(updated_files, [])


if __name__ == "__main__":
    unittest.main()
