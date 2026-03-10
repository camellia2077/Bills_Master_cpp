from __future__ import annotations

import io
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools.reporting.compile2pdf.internal import handlers as facade


class Compile2PdfHandlersTests(unittest.TestCase):
    def test_handlers_facade_preserves_public_imports(self) -> None:
        self.assertTrue(callable(facade.handle_auto))
        self.assertTrue(callable(facade.handle_md))
        self.assertTrue(callable(facade.handle_tex))

    def test_handle_md_routes_to_benchmark_when_multiple_compilers_enabled(self) -> None:
        args = SimpleNamespace(
            markdown_compilers=["pandoc", "typst"],
            compile_types=["markdown"],
            benchmark_loops=1,
            font="Noto Serif SC",
            source_dir="src",
            output_dir="out",
            jobs=None,
            incremental=False,
        )
        with patch(
            "tools.reporting.compile2pdf.internal.handlers_support.format_handlers._run_benchmark",
            return_value=(1, 0, 1.0, []),
        ) as mocked:
            result = facade.handle_md(args)

        self.assertEqual(result, (1, 0, 1.0, []))
        mocked.assert_called_once()

    def test_discover_and_execute_tasks(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            (root / "markdown_notes").mkdir()
            (root / "latex_docs").mkdir()
            compiler_map = {
                ("markdown", "md"): ("Markdown", lambda args: (1, 0, 1.0, ["a.md"])),
                ("latex", "tex"): ("TeX", lambda args: (2, 0, 2.0, ["b.tex"])),
            }

            tasks = facade._discover_tasks(str(root), compiler_map, ["Markdown", "TeX"])
            timing, stats, updated = facade._execute_tasks(
                tasks,
                SimpleNamespace(source_dir=str(root), compile_types=["Markdown", "TeX"]),
            )

        self.assertEqual(len(tasks), 2)
        self.assertEqual(timing["Markdown"], (1.0, 1))
        self.assertEqual(stats["TeX"]["success"], 2)
        self.assertEqual(updated["TeX"], 1)

    def test_handle_auto_prints_aggregated_summaries(self) -> None:
        args = SimpleNamespace(source_dir="src", compile_types=["Markdown"])
        buffer = io.StringIO()
        with redirect_stdout(buffer), patch(
            "tools.reporting.compile2pdf.internal.handlers_support.auto_mode._discover_tasks",
            return_value=[{"log_name": "Markdown", "handler_func": lambda args: (1, 0, 2.0, ["a"]), "source_path": "src/markdown"}],
        ), patch(
            "tools.reporting.compile2pdf.internal.handlers_support.auto_mode._execute_tasks",
            return_value=({"Markdown": (2.0, 1)}, {"Markdown": {"success": 1, "failed": 0}}, {"Markdown": 1}),
        ):
            facade.handle_auto(args)

        output = buffer.getvalue()
        self.assertIn("启动自动编译模式", output)
        self.assertIn("编译时间摘要", output)
        self.assertIn("最终编译统计报告", output)


if __name__ == "__main__":
    unittest.main()
