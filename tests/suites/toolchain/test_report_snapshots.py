from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.verify.report_snapshot_support import (
    COMPARE_SCOPE_STANDARD_REPORT,
    normalize_render_json,
    normalize_standard_report_json,
)
from tools.verify.workflows.bills_tracer import run_bills_tracer_workflow


class ReportSnapshotSupportTests(unittest.TestCase):
    def test_render_json_normalization_preserves_input_key_order(self) -> None:
        raw_json = (
            '{"meta":{"generated_at_utc":"2026-03-11T00:00:00Z"},'
            '"zeta":1,"alpha":2}'
        )

        normalized = normalize_render_json(raw_json)

        self.assertNotIn("generated_at_utc", normalized)
        self.assertLess(normalized.index('"zeta"'), normalized.index('"alpha"'))

    def test_standard_report_normalization_sorts_keys(self) -> None:
        raw_json = (
            '{"meta":{"generated_at_utc":"2026-03-11T00:00:00Z"},'
            '"zeta":1,"alpha":2}'
        )

        normalized = normalize_standard_report_json(raw_json)

        self.assertNotIn("generated_at_utc", normalized)
        self.assertLess(normalized.index('"alpha"'), normalized.index('"zeta"'))


class BillsTracerVerifyWorkflowTests(unittest.TestCase):
    def test_run_bills_tracer_workflow_adds_compare_steps_for_all_formats(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            captured: dict[str, object] = {}

            def fake_run_pipeline_steps(**kwargs) -> int:
                captured["steps"] = kwargs["steps"]
                return 0

            with patch(
                "tools.verify.workflows.bills_tracer.run_pipeline_steps",
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
                "tools.verify.workflows.bills_tracer.run_pipeline_steps",
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
