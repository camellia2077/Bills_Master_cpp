from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.verify.report_snapshot_support import (
    BYTE_COMPARE_MODE,
    COMPARE_SCOPE_STANDARD_REPORT,
    JSON_CONTENT_COMPARE_MODE,
    MARKDOWN_CONTENT_COMPARE_MODE,
    compare_mode_for_extra_source,
    normalize_json_content,
    normalize_markdown_content,
)
from tools.toolchain.verify.bills_tracer import run_bills_tracer_workflow


class ReportSnapshotSupportTests(unittest.TestCase):
    def test_markdown_normalization_ignores_marker_formatting(self) -> None:
        left = "# Title\n- item\n"
        right = "# Title\n* item\n"

        self.assertEqual(
            normalize_markdown_content(left),
            normalize_markdown_content(right),
        )

    def test_json_normalization_ignores_key_order(self) -> None:
        raw_json = (
            '{"meta":{"generated_at_utc":"2026-03-11T00:00:00Z"},'
            '"zeta":1,"alpha":2}'
        )
        same_content_different_order = (
            '{"alpha":2,"meta":{"generated_at_utc":"2026-03-11T00:00:00Z"},"zeta":1}'
        )

        self.assertEqual(
            normalize_json_content(raw_json),
            normalize_json_content(same_content_different_order),
        )

    def test_extra_source_modes_keep_bytes_for_tex_rst_typ(self) -> None:
        self.assertEqual(compare_mode_for_extra_source("LaTeX_bills/years/2025.tex"), BYTE_COMPARE_MODE)
        self.assertEqual(compare_mode_for_extra_source("reST_bills/years/2025.rst"), BYTE_COMPARE_MODE)
        self.assertEqual(compare_mode_for_extra_source("Typst_bills/years/2025.typ"), BYTE_COMPARE_MODE)

    def test_extra_source_modes_use_content_compare_for_markdown_and_json(self) -> None:
        self.assertEqual(
            compare_mode_for_extra_source("Markdown_bills/years/2025.md"),
            MARKDOWN_CONTENT_COMPARE_MODE,
        )
        self.assertEqual(
            compare_mode_for_extra_source("standard_json/years/2025.json"),
            JSON_CONTENT_COMPARE_MODE,
        )


class BillsTracerVerifyWorkflowTests(unittest.TestCase):
    def test_run_bills_tracer_workflow_adds_compare_steps_for_all_formats(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            captured: dict[str, object] = {}

            def fake_run_pipeline_steps(**kwargs) -> int:
                captured["steps"] = kwargs["steps"]
                return 0

            with patch(
                "tools.toolchain.verify.bills_tracer.run_pipeline_steps",
                side_effect=fake_run_pipeline_steps,
            ):
                code = run_bills_tracer_workflow(repo_root, "python", [])

        self.assertEqual(code, 0)
        steps = captured["steps"]
        self.assertEqual(
            [step["id"] for step in steps],
            [
                "bills_tracer",
                "snapshot_compare_md",
                "snapshot_compare_json",
                "snapshot_compare_tex",
                "snapshot_compare_rst",
                "snapshot_compare_typ",
                "snapshot_compare_standard_report",
            ],
        )

    def test_run_bills_tracer_workflow_limits_compare_steps_to_selected_formats(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            captured: dict[str, object] = {}

            def fake_run_pipeline_steps(**kwargs) -> int:
                captured["steps"] = kwargs["steps"]
                return 0

            with patch(
                "tools.toolchain.verify.bills_tracer.run_pipeline_steps",
                side_effect=fake_run_pipeline_steps,
            ):
                code = run_bills_tracer_workflow(
                    repo_root,
                    "python",
                    ["--formats", "md,json", "--output-project", "demo_project"],
                )

        self.assertEqual(code, 0)
        steps = captured["steps"]
        self.assertEqual(
            [step["id"] for step in steps],
            [
                "bills_tracer",
                "snapshot_compare_md",
                "snapshot_compare_json",
                "snapshot_compare_standard_report",
            ],
        )
        standard_report_step = steps[-1]
        self.assertEqual(
            standard_report_step["command"][-1],
            COMPARE_SCOPE_STANDARD_REPORT,
        )


if __name__ == "__main__":
    unittest.main()
