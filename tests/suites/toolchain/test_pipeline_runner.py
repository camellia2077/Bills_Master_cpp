from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.verify.pipeline_runner_support.artifacts import (
    evaluate_artifacts,
    resolve_output_root,
    sync_latest,
)
from tools.verify.pipeline_runner_support.config_loader import build_step_specs
from tools.verify.pipeline_runner_support.dag import topo_sort_steps


class PipelineRunnerTests(unittest.TestCase):
    def test_build_step_specs_resolves_templates(self) -> None:
        repo_root = Path("C:/repo")
        steps = build_step_specs(
            raw_steps=[
                {
                    "id": "demo",
                    "command": ["{python}", "{repo_root}/tools/demo.py"],
                    "cwd": "{repo_root}/tools",
                    "env": {"ROOT": "{repo_root}"},
                    "artifacts": ["dist/tests/demo/latest/output.json"],
                }
            ],
            repo_root=repo_root,
            python_exe="python.exe",
            default_timeout_seconds=60,
        )

        self.assertEqual(len(steps), 1)
        self.assertEqual(steps[0].command, ["python.exe", "C:/repo/tools/demo.py"])
        self.assertEqual(steps[0].cwd, "C:/repo/tools")
        self.assertEqual(steps[0].env, {"ROOT": "C:/repo"})
        self.assertEqual(steps[0].timeout_seconds, 60)

    def test_topo_sort_steps_orders_dependencies(self) -> None:
        repo_root = Path("C:/repo")
        steps = build_step_specs(
            raw_steps=[
                {"id": "third", "command": ["echo", "3"], "depends_on": ["second"]},
                {"id": "first", "command": ["echo", "1"]},
                {"id": "second", "command": ["echo", "2"], "depends_on": ["first"]},
            ],
            repo_root=repo_root,
            python_exe="python.exe",
            default_timeout_seconds=60,
        )

        ordered = topo_sort_steps(steps)

        self.assertEqual([step.step_id for step in ordered], ["first", "second", "third"])

    def test_evaluate_artifacts_and_output_root(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            artifact = repo_root / "dist" / "tests" / "artifact.json"
            artifact.parent.mkdir(parents=True, exist_ok=True)
            artifact.write_text("demo", encoding="utf-8")

            results = evaluate_artifacts(repo_root, ["dist/tests/artifact.json"])
            output_root = resolve_output_root(
                repo_root,
                {"root": "dist/tests/logic/custom_pipeline"},
                "demo",
            )

        self.assertEqual(
            results,
            [
                {
                    "path": artifact.as_posix(),
                    "exists": True,
                    "size_bytes": 4,
                }
            ],
        )
        self.assertEqual(output_root, (repo_root / "dist" / "tests" / "logic" / "custom_pipeline"))

    def test_sync_latest_projects_logs_and_manifests(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            output_root = root / "dist" / "tests" / "logic" / "pipeline"
            run_dir = output_root / "runs" / "run_001"
            (run_dir / "logs").mkdir(parents=True, exist_ok=True)
            (run_dir / "logs" / "step.log").write_text("ok", encoding="utf-8")
            (run_dir / "pipeline_summary.json").write_text("{}", encoding="utf-8")
            (run_dir / "run_manifest.json").write_text("{}", encoding="utf-8")

            sync_latest(output_root, run_dir)

            latest_run = (output_root / "latest_run.txt").read_text(encoding="utf-8")
            summary_exists = (output_root / "pipeline_summary.json").exists()
            manifest_exists = (output_root / "run_manifest.json").exists()
            step_log_exists = (output_root / "logs" / "step.log").exists()

        self.assertEqual(latest_run, "run_001")
        self.assertTrue(summary_exists)
        self.assertTrue(manifest_exists)
        self.assertTrue(step_log_exists)


if __name__ == "__main__":
    unittest.main()
