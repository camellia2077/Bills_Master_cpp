from __future__ import annotations

import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

from tools.verify.checks.check_boundary_layering_support.metadata import (
    load_allowlist,
    write_baseline,
)
from tools.verify.checks.check_boundary_layering_support.policy import check_policy, print_stats
from tools.verify.checks.check_boundary_layering_support.scan import scan_scope


class BoundaryLayeringTests(unittest.TestCase):
    def test_scan_scope_collects_include_records(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "bills_core" / "src" / "abi" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\n#include <vector>\n', encoding="utf-8")

            includes, scanned_files = scan_scope(
                repo_root,
                "core_abi",
                "libs/bills_core/src/abi",
            )

        self.assertEqual(scanned_files, 1)
        self.assertEqual(len(includes), 2)
        self.assertEqual(includes[0].header, "demo.h")

    def test_write_baseline_and_load_allowlist_preserve_schema(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "bills_core" / "src" / "abi" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\n', encoding="utf-8")
            includes, _ = scan_scope(repo_root, "core_abi", "libs/bills_core/src/abi")
            config_path = repo_root / "boundary.json"

            write_baseline(config_path, includes, "owner_demo")
            allowlist, errors, tier_map = load_allowlist(config_path)
            payload = json.loads(config_path.read_text(encoding="utf-8"))

        self.assertEqual(errors, [])
        self.assertIn(("libs/bills_core/src/abi/demo.cpp", "demo.h"), allowlist)
        self.assertEqual(payload["schema_version"], 1)
        self.assertEqual(tier_map[("libs/bills_core/src/abi/demo.cpp", "demo.h")], "replaceable")

    def test_check_policy_and_stats_report_violations(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "libs" / "bills_core" / "src" / "abi" / "demo.cpp"
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text('#include "demo.h"\n', encoding="utf-8")
            includes, scanned_files = scan_scope(repo_root, "core_abi", "libs/bills_core/src/abi")

            violations, observed = check_policy(includes, allowlist=set())
            buffer = io.StringIO()
            with redirect_stdout(buffer):
                with patch.dict(
                    "tools.verify.checks.check_boundary_layering_support.policy.BOUNDARY_LAYER_ROOTS",
                    {"core_abi": "libs/bills_core/src/abi"},
                    clear=True,
                ):
                    print_stats(includes, {"core_abi": scanned_files}, observed, {})

        self.assertEqual(len(violations), 1)
        self.assertIn("core_abi,1,1,1", buffer.getvalue())


if __name__ == "__main__":
    unittest.main()
