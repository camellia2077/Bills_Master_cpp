from __future__ import annotations

import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

from tools.verify.checks.check_import_layering_support.metadata import (
    load_allowlist,
    write_baseline,
)
from tools.verify.checks.check_import_layering_support.policy import check_policy, print_stats
from tools.verify.checks.check_import_layering_support.scan import scan_scope


class ImportLayeringTests(unittest.TestCase):
    def test_scan_scope_collects_include_and_import_records(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "core" / "src" / "application" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\nimport std;\n', encoding="utf-8")

            includes, imports, scanned_files = scan_scope(
                repo_root,
                "core_application",
                "libs/core/src/application",
            )

        self.assertEqual(scanned_files, 1)
        self.assertEqual(len(includes), 1)
        self.assertEqual(len(imports), 1)
        self.assertEqual(imports[0].line, 2)

    def test_write_baseline_and_load_allowlist_preserve_schema(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "core" / "src" / "application" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\n', encoding="utf-8")
            includes, _, _ = scan_scope(repo_root, "core_application", "libs/core/src/application")
            config_path = repo_root / "import.json"

            write_baseline(repo_root, config_path, includes, "owner_demo", "reason_demo")
            allowlist, errors = load_allowlist(config_path)
            payload = json.loads(config_path.read_text(encoding="utf-8"))

        self.assertEqual(errors, [])
        self.assertIn(("libs/core/src/application/demo.cpp", "demo.h"), allowlist)
        self.assertEqual(payload["schema_version"], 1)

    def test_check_policy_and_stats_report_violations(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "core" / "src" / "application" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\nimport std;\n', encoding="utf-8")
            includes, imports, scanned_files = scan_scope(
                repo_root,
                "core_application",
                "libs/core/src/application",
            )

            violations, observed = check_policy(includes, allowlist=set())
            del observed
            buffer = io.StringIO()
            with redirect_stdout(buffer):
                with patch.dict(
                    "tools.verify.checks.check_import_layering_support.policy.CALL_LAYER_ROOTS",
                    {"core_application": "libs/core/src/application"},
                    clear=True,
                ):
                    print_stats(includes, imports, {"core_application": scanned_files})

        self.assertEqual(len(violations), 1)
        self.assertIn("core_application,1,1,1,1", buffer.getvalue())


if __name__ == "__main__":
    unittest.main()
