from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.flows.bills_tracer_flow_support.cmake_dist import resolve_build_dir
from tools.flows.bills_tracer_flow_support.config_distribution import (
    ANDROID_TARGET,
    WINDOWS_TARGET,
    distribute_configs,
    load_enabled_formats,
)
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
        repo_root = Path(__file__).resolve().parents[3]
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
                target="bills-tracer-cli",
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
                import_dir=root / "runtime" / "cache" / "txt2json",
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
        self.assertIn(
            "dirs_to_delete = ['db', 'cache', 'exports', 'logs', 'record_templates', 'config', 'notices']",
            content,
        )

    def test_distribute_configs_preserves_windows_and_filters_android_formats(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_root = root / "config"
            output_root = root / "dist" / "config"
            source_root.mkdir(parents=True, exist_ok=True)
            validator_text = "[[categories]]\nparent_item = \"meal\"\n"
            modifier_text = "metadata_prefixes = [\"date:\", \"remark:\"]\n"
            export_text = "enabled_formats = [\"json\", \"md\", \"rst\", \"tex\", \"typ\"]\n"
            (source_root / "validator_config.toml").write_text(validator_text, encoding="utf-8")
            (source_root / "modifier_config.toml").write_text(modifier_text, encoding="utf-8")
            (source_root / "export_formats.toml").write_text(export_text, encoding="utf-8")

            outputs = distribute_configs(source_root, output_root, [WINDOWS_TARGET, ANDROID_TARGET])

            self.assertEqual(
                export_text,
                (outputs[WINDOWS_TARGET] / "export_formats.toml").read_text(encoding="utf-8"),
            )
            self.assertEqual(
                validator_text,
                (outputs[ANDROID_TARGET] / "validator_config.toml").read_text(encoding="utf-8"),
            )
            self.assertEqual(
                modifier_text,
                (outputs[ANDROID_TARGET] / "modifier_config.toml").read_text(encoding="utf-8"),
            )
            self.assertEqual(
                ["json", "md"],
                load_enabled_formats(outputs[ANDROID_TARGET] / "export_formats.toml"),
            )

    def test_distribute_configs_preserves_source_format_order(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_root = root / "config"
            output_root = root / "dist" / "config"
            source_root.mkdir(parents=True, exist_ok=True)
            (source_root / "validator_config.toml").write_text("[[categories]]\n", encoding="utf-8")
            (source_root / "modifier_config.toml").write_text("metadata_prefixes = []\n", encoding="utf-8")
            (source_root / "export_formats.toml").write_text(
                "enabled_formats = [\"md\", \"typ\", \"json\", \"tex\"]\n",
                encoding="utf-8",
            )

            outputs = distribute_configs(source_root, output_root, [ANDROID_TARGET])

            self.assertEqual(
                ["md", "json"],
                load_enabled_formats(outputs[ANDROID_TARGET] / "export_formats.toml"),
            )


if __name__ == "__main__":
    unittest.main()
