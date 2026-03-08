from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.flows.bills_tracer_flow_support.cmake_dist import resolve_build_dir
from tools.flows.bills_tracer_flow_support.config_writer import (
    parse_formats,
    write_temp_test_config,
)
from tools.toolchain.services.build_layout import (
    resolve_build_directory,
    sanitize_segment,
    short_hash,
)


class BillsTracerFlowSupportTests(unittest.TestCase):
    def test_parse_formats_normalizes_and_deduplicates(self) -> None:
        self.assertEqual(
            parse_formats(" MD, json ,md,tex,rst "),
            ["md", "json", "tex", "rst"],
        )

    def test_resolve_build_dir_uses_stable_isolated_hash(self) -> None:
        repo_root = Path(__file__).resolve().parents[2]
        expected_seed = "|".join(
            [
                sanitize_segment("bills_tracer_model"),
                sanitize_segment("json-first"),
                sanitize_segment("md-json"),
                "debug",
                sanitize_segment("ninja"),
            ]
        )
        expected_instance = f"b_{short_hash(expected_seed, length=12)}"

        build_dir = resolve_build_dir(
            repo_root=repo_root,
            build_scope="isolated",
            build_preset="debug",
            output_project="bills_tracer_model",
            export_pipeline="json-first",
            formats=["md", "json"],
            generator="Ninja",
        )

        self.assertEqual(
            build_dir,
            resolve_build_directory(
                repo_root,
                target="bills",
                preset="debug",
                scope="isolated",
                instance_id=expected_instance,
            ).build_dir,
        )

    def test_write_temp_test_config_renders_workspace_contract(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config_path = root / "runtime_config.toml"
            write_temp_test_config(
                config_path=config_path,
                workspace_dir=root / "workspace",
                bills_dir=root / "testdata" / "bills",
                import_dir=root / "runtime" / "txt2josn",
                runtime_base_dir=root / "runtime",
                runtime_run_id="run_123",
                runtime_output_dir=root / "artifact",
                runtime_summary_path=root / "artifact" / "test_summary.json",
                run_export_all_tasks=True,
                export_formats=["md", "json"],
                ingest_mode="stepwise",
                ingest_write_json=False,
                export_pipeline="model-first",
                output_project="bills_tracer",
                single_year="2025",
                single_month="2025-01",
                range_start="2025-03",
                range_end="2025-04",
            )

            content = config_path.read_text(encoding="utf-8")

        self.assertIn("[paths]", content)
        self.assertIn("workspace_dir = '", content)
        self.assertIn("export_formats = ['md', 'json']", content)
        self.assertIn("dirs_to_delete = ['dist', 'config', 'output']", content)


if __name__ == "__main__":
    unittest.main()
