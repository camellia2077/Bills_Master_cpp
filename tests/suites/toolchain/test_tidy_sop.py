from __future__ import annotations

import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools.toolchain.cli.main import parse_cli_args
from tools.toolchain.commands.tidy_batch import execute_tidy_batch
from tools.toolchain.commands.tidy_fix import TidyFixResult
from tools.toolchain.commands.tidy_show import run as run_tidy_show
from tools.toolchain.commands.tidy_status import _print_batch_status
from tools.toolchain.core.config import ToolchainConfig, load_toolchain_config
from tools.toolchain.core.context import Context
from tools.toolchain.core.process_runner import ProcessRunner
from tools.toolchain.services.clang_tidy_runner import normalize_checks_filter
from tools.toolchain.services.fix_strategy import summarize_checks
from tools.toolchain.services.tidy_paths import resolve_tidy_paths
from tools.toolchain.services.tidy_residuals import classify_residual_diagnostics
from tools.toolchain.services.tidy_runtime import (
    build_numbering_context,
    load_batch_runtime_state,
    update_batch_runtime_state,
    write_tidy_result,
)


class TidySopTests(unittest.TestCase):
    def test_dist_parser_does_not_reforward_owned_flags(self) -> None:
        _, args = parse_cli_args(
            ["dist", "bills-tracer-core", "--preset", "debug", "--scope", "shared"]
        )
        self.assertEqual(args.forwarded, [])

    def test_dist_parser_preserves_explicit_forwarded_args(self) -> None:
        _, args = parse_cli_args(["dist", "bills-tracer-core", "--preset", "debug", "--modules"])
        self.assertEqual(args.forwarded, ["--modules"])

    def test_load_toolchain_config_reads_new_tidy_sections(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config_path = root / "workflow.toml"
            config_path.write_text(
                """
[dist]
default_target = "bills-tracer-core"

[tidy.safe_fix_prepass]
checks = ["modernize-use-trailing-return-type"]

[tidy.suppression]
mode = "suggest_only"
allowed_checks = ["readability-function-cognitive-complexity"]

[tidy.status]
explain_closed_ranges = false
""".strip(),
                encoding="utf-8",
            )
            config = load_toolchain_config(config_path)

        self.assertEqual(config.dist.default_target, "bills-tracer-core")
        self.assertEqual(
            config.tidy.safe_fix_prepass.checks,
            ["modernize-use-trailing-return-type"],
        )
        self.assertEqual(config.tidy.suppression.mode, "suggest_only")
        self.assertEqual(
            config.tidy.suppression.allowed_checks,
            ["readability-function-cognitive-complexity"],
        )
        self.assertFalse(config.tidy.status.explain_closed_ranges)

    def test_load_toolchain_config_rejects_legacy_build_section(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config_path = root / "workflow.toml"
            config_path.write_text(
                """
[build]
default_target = "bills-tracer-cli"
""".strip(),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, r"\[build\]"):
                load_toolchain_config(config_path)

    def test_tidy_runtime_facade_preserves_public_imports(self) -> None:
        from tools.toolchain.services import tidy_runtime as facade

        self.assertIs(facade.build_numbering_context, build_numbering_context)
        self.assertIs(facade.write_tidy_result, write_tidy_result)
        self.assertTrue(callable(facade.load_batch_runtime_state))

    def test_summarize_checks_reports_primary_and_auxiliary_flags(self) -> None:
        config = ToolchainConfig()
        summary = summarize_checks(
            [
                "modernize-use-nullptr",
                "readability-function-cognitive-complexity",
            ],
            strategy_cfg=config.tidy.fix_strategy,
            safe_fix_patterns=config.tidy.safe_fix_prepass.checks,
            suppression_allowed_patterns=config.tidy.suppression.allowed_checks,
        )
        self.assertEqual(summary.primary_strategy, "safe_refactor")
        self.assertEqual(summary.safe_fix_checks_present, ["modernize-use-nullptr"])
        self.assertEqual(
            summary.suppression_candidates_present,
            ["readability-function-cognitive-complexity"],
        )

    def test_normalize_checks_filter_starts_from_disable_all(self) -> None:
        self.assertEqual(
            normalize_checks_filter(
                [
                    "modernize-use-trailing-return-type",
                    "-*",
                    "modernize-use-nodiscard",
                    "modernize-use-trailing-return-type",
                ]
            ),
            [
                "-*",
                "modernize-use-trailing-return-type",
                "modernize-use-nodiscard",
            ],
        )

    def test_classify_residual_diagnostics_assigns_expected_actions(self) -> None:
        diagnostics, summary = classify_residual_diagnostics(
            [
                {
                    "file": "a.cpp",
                    "line": 10,
                    "col": 1,
                    "check": "readability-function-cognitive-complexity",
                    "message": "too complex",
                    "severity": "warning",
                },
                {
                    "file": "b.cpp",
                    "line": 20,
                    "col": 2,
                    "check": "readability-magic-numbers",
                    "message": "magic number",
                    "severity": "warning",
                },
                {
                    "file": "c.cpp",
                    "line": 30,
                    "col": 3,
                    "check": "modernize-use-trailing-return-type",
                    "message": "trailing return type",
                    "severity": "warning",
                },
            ],
            suppression_allowed_patterns=[
                "readability-function-cognitive-complexity",
                "bugprone-easily-swappable-parameters",
            ],
            safe_fix_patterns=["modernize-use-trailing-return-type"],
        )
        self.assertEqual(diagnostics[0]["preferred_action"], "suggest_nolint")
        self.assertEqual(diagnostics[1]["preferred_action"], "manual_refactor")
        self.assertEqual(
            diagnostics[1]["reason_template"],
            "domain constant should be extracted before suppression",
        )
        self.assertEqual(summary["unexpected_fixable_count"], 1)

    def test_classify_residual_diagnostics_can_skip_unexpected_safe_fix_count(self) -> None:
        diagnostics, summary = classify_residual_diagnostics(
            [
                {
                    "file": "a.cpp",
                    "line": 10,
                    "col": 1,
                    "check": "modernize-use-trailing-return-type",
                    "message": "trailing return type",
                    "severity": "warning",
                }
            ],
            suppression_allowed_patterns=[],
            safe_fix_patterns=["modernize-use-trailing-return-type"],
            count_safe_fix_as_unexpected=False,
        )
        self.assertEqual(diagnostics[0]["preferred_action"], "manual_refactor")
        self.assertEqual(summary["unexpected_fixable_count"], 0)

    def test_build_numbering_context_reports_closed_ranges(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            for batch_number in range(1, 13):
                batch_dir = paths.tasks_done_dir / f"batch_{batch_number:03d}"
                batch_dir.mkdir(parents=True, exist_ok=True)
                (batch_dir / f"task_{batch_number:03d}.log").write_text("done", encoding="utf-8")

            numbering = build_numbering_context(paths, current_batch_id="batch_013")

        self.assertEqual(numbering["already_closed_before_current"], 12)
        self.assertEqual(numbering["already_closed_ranges"], ["batch_001..batch_012"])
        self.assertEqual(numbering["next_open_batch"], "batch_013")

    def test_write_tidy_result_projects_latest_and_legacy_views(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            for batch_number in range(1, 13):
                batch_dir = paths.tasks_done_dir / f"batch_{batch_number:03d}"
                batch_dir.mkdir(parents=True, exist_ok=True)
                (batch_dir / f"task_{batch_number:03d}.log").write_text("done", encoding="utf-8")
            update_batch_runtime_state(
                paths,
                "batch_013",
                status="needs_manual",
                current_phase="classify",
                source_files=["C:/repo/file.cpp"],
                decision_summary={
                    "manual_refactor_count": 1,
                    "suggest_nolint_count": 0,
                    "unexpected_fixable_count": 0,
                    "files_with_remaining": ["C:/repo/file.cpp"],
                },
                remaining={
                    "count": 1,
                    "diagnostics": [
                        {
                            "file": "C:/repo/file.cpp",
                            "line": 10,
                            "col": 1,
                            "check": "readability-function-cognitive-complexity",
                            "message": "too complex",
                            "severity": "warning",
                            "preferred_action": "suggest_nolint",
                            "fallback_action": None,
                            "reason_template": "external behavior preserved; local extraction too invasive for current batch",
                        }
                    ],
                },
                numbering_context=build_numbering_context(paths, current_batch_id="batch_013"),
            )
            write_tidy_result(
                ctx,
                paths,
                stage="tidy-batch",
                status="needs_manual",
                exit_code=1,
                batch_id="batch_013",
                current_phase="classify",
            )

            latest = json.loads(paths.latest_state_path.read_text(encoding="utf-8"))
            legacy = json.loads(paths.tidy_result_path.read_text(encoding="utf-8"))

        self.assertEqual(latest["current_batch"], "batch_013")
        self.assertEqual(latest["current_phase"], "classify")
        self.assertEqual(
            latest["numbering_context"]["already_closed_ranges"],
            ["batch_001..batch_012"],
        )
        self.assertEqual(legacy["status"], "needs_manual")
        self.assertEqual(
            legacy["next_action"],
            "Next: python tools/run.py tidy-status --batch-id batch_013",
        )

    def test_write_tidy_result_does_not_escalate_workflow_bug_without_prepass(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            update_batch_runtime_state(
                paths,
                "batch_013",
                status="needs_manual",
                current_phase="classify",
                auto_fix_prepass={
                    "status": "not_run",
                    "checks": [],
                    "matched_checks_present": [],
                    "target_files": [],
                    "changed_files": [],
                    "log_path": None,
                    "returncode": None,
                },
                decision_summary={
                    "manual_refactor_count": 1,
                    "suggest_nolint_count": 0,
                    "unexpected_fixable_count": 1,
                    "files_with_remaining": ["C:/repo/file.cpp"],
                },
                numbering_context=build_numbering_context(paths, current_batch_id="batch_013"),
            )
            write_tidy_result(
                ctx,
                paths,
                stage="tidy-recheck",
                status="needs_manual",
                exit_code=0,
                batch_id="batch_013",
                current_phase="classify",
            )
            latest = json.loads(paths.latest_state_path.read_text(encoding="utf-8"))

        self.assertEqual(
            latest["next_action"],
            "Next: python tools/run.py tidy-status --batch-id batch_013",
        )

    def test_tidy_status_batch_view_recomputes_empty_numbering_context(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            for batch_number in range(1, 13):
                batch_dir = paths.tasks_done_dir / f"batch_{batch_number:03d}"
                batch_dir.mkdir(parents=True, exist_ok=True)
                (batch_dir / f"task_{batch_number:03d}.log").write_text("done", encoding="utf-8")

            buffer = io.StringIO()
            with redirect_stdout(buffer):
                exit_code = _print_batch_status(paths, "batch_013")

        self.assertEqual(exit_code, 0)
        output = buffer.getvalue()
        self.assertIn("next_open_batch=batch_013", output)
        self.assertIn("already_closed_ranges=batch_001..batch_012", output)

    def test_tidy_show_batch_view_recomputes_empty_numbering_context(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            for batch_number in range(1, 13):
                batch_dir = paths.tasks_done_dir / f"batch_{batch_number:03d}"
                batch_dir.mkdir(parents=True, exist_ok=True)
                (batch_dir / f"task_{batch_number:03d}.log").write_text("done", encoding="utf-8")

            buffer = io.StringIO()
            with redirect_stdout(buffer):
                exit_code = run_tidy_show(
                    SimpleNamespace(batch_id="batch_013"),
                    ctx,
                )

        self.assertEqual(exit_code, 0)
        self.assertIn(
            "already_closed_ranges=batch_001..batch_012",
            buffer.getvalue(),
        )

    def test_tidy_batch_build_gate_failure_clears_stale_followup_state(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            ctx = self._make_context(Path(temp_dir))
            paths = resolve_tidy_paths(ctx)
            self._write_manifest(paths, pending_batch_ids=["batch_013"])
            update_batch_runtime_state(
                paths,
                "batch_013",
                status="needs_manual",
                current_phase="classify",
                recheck={
                    "status": "completed",
                    "files": ["C:/repo/file.cpp"],
                    "log_path": "temp/recheck.log",
                    "diagnostics_path": "temp/recheck.jsonl",
                    "returncode": 0,
                    "diagnostics_count": 3,
                },
                remaining={
                    "count": 3,
                    "diagnostics": [
                        {
                            "file": "C:/repo/file.cpp",
                            "line": 10,
                            "col": 1,
                            "check": "modernize-use-trailing-return-type",
                            "message": "old residual",
                            "severity": "warning",
                            "preferred_action": "manual_refactor",
                            "fallback_action": None,
                            "reason_template": "manual source change required; no narrow suppression rule is configured",
                        }
                    ],
                },
                decision_summary={
                    "manual_refactor_count": 3,
                    "suggest_nolint_count": 0,
                    "unexpected_fixable_count": 3,
                    "files_with_remaining": ["C:/repo/file.cpp"],
                },
            )
            fix_result = TidyFixResult(
                returncode=0,
                log_path=paths.refresh_dir / "batch_013_fix.log",
                target_files=[Path("C:/repo/file.cpp")],
                changed_files=[Path("C:/repo/file.cpp")],
                checks_filter=["modernize-use-trailing-return-type"],
            )
            with (
                patch(
                    "tools.toolchain.commands.tidy_batch_support.steps.run_tidy_fix_pass",
                    return_value=fix_result,
                ),
                patch(
                    "tools.toolchain.commands.tidy_batch_support.steps.run_verify_workflow",
                    return_value=(
                        [
                            "python",
                            "verify.py",
                            "bills-tracer-cli-dist",
                            "--",
                            "--preset",
                            "debug",
                            "--scope",
                            "shared",
                        ],
                        1,
                    ),
                ),
            ):
                exit_code = execute_tidy_batch(
                    ctx,
                    batch_id="batch_013",
                    run_verify=False,
                )
            state = load_batch_runtime_state(paths, "batch_013")

        self.assertEqual(exit_code, 1)
        self.assertEqual(state["status"], "needs_manual_after_fix")
        self.assertEqual(state["current_phase"], "build_gate")
        self.assertEqual(state["recheck"]["status"], "skipped")
        self.assertEqual(state["recheck"]["diagnostics_count"], 0)
        self.assertEqual(state["remaining"]["count"], 0)
        self.assertEqual(
            state["decision_summary"]["unexpected_fixable_count"],
            0,
        )

    def _make_context(self, root: Path) -> Context:
        tools_root = root / "tools"
        tools_root.mkdir(parents=True, exist_ok=True)
        temp_root = root / "temp"
        temp_root.mkdir(parents=True, exist_ok=True)
        return Context(
            repo_root=root,
            tools_root=tools_root,
            temp_root=temp_root,
            python_executable="python",
            config_path=tools_root / "toolchain" / "config" / "workflow.toml",
            config=ToolchainConfig(),
            process_runner=ProcessRunner(),
        )

    def _write_manifest(self, paths, *, pending_batch_ids: list[str]) -> None:
        tasks = []
        batches = []
        for offset, batch_id in enumerate(pending_batch_ids, start=1):
            task_id = f"{100 + offset:03d}"
            task_path = paths.tasks_dir / batch_id
            task_path.mkdir(parents=True, exist_ok=True)
            log_path = task_path / f"task_{task_id}.log"
            log_path.write_text("File: C:/repo/file.cpp\n", encoding="utf-8")
            tasks.append(
                {
                    "task_id": task_id,
                    "batch_id": batch_id,
                    "source_file": "C:/repo/file.cpp",
                    "score": 1.0,
                    "size": 10,
                    "checks": ["readability-function-cognitive-complexity"],
                    "diagnostic_count": 1,
                    "primary_fix_strategy": "safe_refactor",
                    "safe_fix_checks_present": [],
                    "suppression_candidates_present": ["readability-function-cognitive-complexity"],
                    "content_path": f"tasks/{batch_id}/task_{task_id}.log",
                }
            )
            batches.append(
                {
                    "batch_id": batch_id,
                    "task_count": 1,
                    "checks": ["readability-function-cognitive-complexity"],
                    "files": ["C:/repo/file.cpp"],
                    "primary_fix_strategy": ["safe_refactor"],
                    "safe_fix_checks_present": [],
                    "suppression_candidates_present": ["readability-function-cognitive-complexity"],
                }
            )
        payload = {
            "generated_at": "2026-03-08T00:00:00+00:00",
            "task_count": len(tasks),
            "batch_count": len(batches),
            "batch_size": 10,
            "tasks": tasks,
            "batches": batches,
        }
        paths.tasks_manifest.write_text(
            json.dumps(payload, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )


if __name__ == "__main__":
    unittest.main()
