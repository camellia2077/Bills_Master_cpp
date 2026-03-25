from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.verify.workflows.cli import build_parser
from tools.verify.workflows.common import load_python_test_duration_seconds, load_test_summary
from tools.verify.workflows.pipeline_helpers import run_pipeline_steps
from tools.verify.workflows.registry import workflow_help_text, workflow_registry, workflow_specs


class VerifyCliTests(unittest.TestCase):
    def test_workflow_registry_matches_declared_specs(self) -> None:
        specs = workflow_specs()
        registry = workflow_registry()

        self.assertEqual([spec.name for spec in specs], list(registry.keys()))
        self.assertEqual(specs[0].name, "bills-tracer")
        self.assertIn("bills-tracer-log-generator-dist", registry)
        self.assertIn("reporting-tools", registry)

    def test_parser_uses_registry_choices_and_help_text(self) -> None:
        parser = build_parser()
        workflow_action = next(action for action in parser._actions if action.dest == "workflow")

        self.assertEqual(list(workflow_action.choices), [spec.name for spec in workflow_specs()])
        self.assertEqual(workflow_action.help, workflow_help_text())

    def test_run_pipeline_steps_builds_temp_config_and_runner_command(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            captured: dict[str, object] = {}

            def fake_run(command: list[str]) -> int:
                captured["command"] = command
                config_path = Path(command[3])
                captured["config_text"] = config_path.read_text(encoding="utf-8")
                return 0

            with patch("tools.verify.workflows.pipeline_helpers.run", side_effect=fake_run):
                code = run_pipeline_steps(
                    repo_root=repo_root,
                    python_exe="python",
                    pipeline_name="verify_bills_tracer",
                    pipeline_description="verify workflow bills_tracer",
                    output_root=str(repo_root / "dist" / "tests"),
                    steps=[
                        {
                            "id": "bills",
                            "command": ["python", "tools/flows/bills_tracer_flow.py"],
                            "artifacts": ["dist/tests/artifact/demo/latest/test_summary.json"],
                        }
                    ],
                    run_id_prefix="demo",
                )

        self.assertEqual(code, 0)
        self.assertEqual(
            captured["command"][:3],
            ["python", str(repo_root / "tools" / "verify" / "pipeline_runner.py"), "--config"],
        )
        self.assertIn('--run-id', captured["command"])
        self.assertIn('name = "verify_bills_tracer"', captured["config_text"])
        self.assertIn('id = "bills"', captured["config_text"])

    def test_summary_and_duration_helpers_read_latest_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            latest_dir = repo_root / "dist" / "tests" / "artifact" / "demo" / "latest"
            latest_dir.mkdir(parents=True, exist_ok=True)
            (latest_dir / "test_summary.json").write_text(
                json.dumps({"ok": True, "success": 3, "failed": 0, "total": 3}),
                encoding="utf-8",
            )
            (latest_dir / "test_python_output.log").write_text(
                "\n".join(
                    [
                        "started_at=2026-03-09T10:00:00",
                        "completed_at=2026-03-09T10:00:01.500000",
                        "",
                    ]
                ),
                encoding="utf-8",
            )

            summary = load_test_summary(repo_root, "demo")
            duration = load_python_test_duration_seconds(repo_root, "demo")

        self.assertEqual(summary, {"ok": True, "success": 3, "failed": 0, "total": 3})
        self.assertAlmostEqual(duration or 0.0, 1.5)


if __name__ == "__main__":
    unittest.main()
