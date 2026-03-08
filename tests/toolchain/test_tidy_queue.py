from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from tools.toolchain.services import tidy_queue as facade
from tools.toolchain.services.tidy_queue_support.builders import split_log_to_tasks


class TidyQueueTests(unittest.TestCase):
    def test_tidy_queue_facade_preserves_public_imports(self) -> None:
        self.assertIs(facade.split_log_to_tasks, split_log_to_tasks)
        self.assertTrue(callable(facade.load_manifest))
        self.assertTrue(callable(facade.next_open_batch))

    def test_split_log_to_tasks_rejects_non_positive_limits(self) -> None:
        ctx = SimpleNamespace(
            config=SimpleNamespace(
                tidy=SimpleNamespace(
                    max_lines=10,
                    max_diags=10,
                    fix_strategy=SimpleNamespace(),
                    safe_fix_prepass=SimpleNamespace(checks=[]),
                    suppression=SimpleNamespace(allowed_checks=[]),
                )
            )
        )

        with self.assertRaisesRegex(ValueError, "max_lines and max_diags"):
            facade.split_log_to_tasks(ctx, log_content="", max_lines=0, max_diags=1)

    def test_manifest_roundtrip_and_queries(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            tasks_dir = root / "tasks"
            manifest_path = root / "tasks_manifest.json"
            summary_path = root / "tasks_summary.md"

            payload = facade.write_manifest_from_entries(
                tasks_manifest=manifest_path,
                tasks_summary_path=summary_path,
                batch_size=2,
                entries=[
                    {
                        "task_id": "001",
                        "batch_id": "batch_001",
                        "source_file": "a.cpp",
                        "score": 1.0,
                        "checks": ["demo-check"],
                        "primary_fix_strategy": "manual_refactor",
                        "safe_fix_checks_present": [],
                        "suppression_candidates_present": [],
                        "content_path": "tasks/batch_001/task_001.log",
                    },
                    {
                        "task_id": "002",
                        "batch_id": "batch_002",
                        "source_file": "b.cpp",
                        "score": 2.0,
                        "checks": ["demo-check-2"],
                        "primary_fix_strategy": "safe_refactor",
                        "safe_fix_checks_present": [],
                        "suppression_candidates_present": [],
                        "content_path": "tasks/batch_002/task_002.log",
                    },
                ],
            )
            (tasks_dir / "batch_001").mkdir(parents=True, exist_ok=True)
            (tasks_dir / "batch_001" / "task_001.log").write_text("a", encoding="utf-8")
            (tasks_dir / "batch_002").mkdir(parents=True, exist_ok=True)
            (tasks_dir / "batch_002" / "task_002.log").write_text("b", encoding="utf-8")
            done_dir = root / "done" / "batch_003"
            done_dir.mkdir(parents=True, exist_ok=True)
            (done_dir / "task_003.log").write_text("done", encoding="utf-8")

            manifest = facade.load_manifest(manifest_path)
            batch_tasks = facade.load_batch_tasks(manifest_path, "batch_001")
            next_batch = facade.next_open_batch(manifest_path)
            indices = facade.max_indices(manifest_path, root / "done")
            removed = facade.remove_tasks_for_files(tasks_dir, manifest_path, {"a.cpp"})
            remaining = facade.load_manifest(manifest_path)

        self.assertEqual(payload["task_count"], 2)
        self.assertEqual(len(manifest["tasks"]), 2)
        self.assertEqual(len(batch_tasks), 1)
        self.assertEqual(next_batch, "batch_001")
        self.assertEqual(indices, (3, 3))
        self.assertEqual(len(removed), 1)
        self.assertEqual(len(remaining["tasks"]), 1)


if __name__ == "__main__":
    unittest.main()
