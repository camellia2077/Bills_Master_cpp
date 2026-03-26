from __future__ import annotations

import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from tools.toolchain.cli.main import parse_cli_args
from tools.toolchain.commands.build import run as run_build
from tools.toolchain.services.dist.android import parse_android_args, resolve_requested_variants


class DistAndroidTests(unittest.TestCase):
    def test_dist_parser_accepts_android_target_and_forwards_unknown_args(self) -> None:
        _, args = parse_cli_args(
            ["dist", "bills-tracer-android", "--preset", "release", "--clean"]
        )

        self.assertEqual(args.target, "bills-tracer-android")
        self.assertEqual(args.preset, "release")
        self.assertEqual(args.scope, "shared")
        self.assertEqual(args.forwarded, ["--clean"])

    def test_run_build_routes_android_to_android_dist_service(self) -> None:
        ctx = SimpleNamespace(
            repo_root=Path("C:/repo"),
        )
        args = SimpleNamespace(
            target="bills-tracer-android",
            preset="release",
            scope="shared",
            forwarded=["--clean"],
        )

        with patch("tools.toolchain.commands.build.run_android_dist", return_value=0) as mock_run:
            exit_code = run_build(args, ctx)

        self.assertEqual(exit_code, 0)
        mock_run.assert_called_once_with(Path("C:/repo"), ["--preset", "release", "--clean"])

    def test_run_build_rejects_tidy_for_android(self) -> None:
        ctx = SimpleNamespace(
            repo_root=Path("C:/repo"),
        )
        args = SimpleNamespace(
            target="bills-tracer-android",
            preset="tidy",
            scope="shared",
            forwarded=[],
        )

        exit_code = run_build(args, ctx)

        self.assertEqual(exit_code, 2)

    def test_android_build_flow_defaults_to_debug_variant(self) -> None:
        args = parse_android_args([])

        self.assertEqual(resolve_requested_variants(args), ["debug"])

    def test_android_build_flow_accepts_preset_selector(self) -> None:
        args = parse_android_args(["--preset", "release"])

        self.assertEqual(resolve_requested_variants(args), ["release"])

    def test_android_build_flow_rejects_preset_and_variants_together(self) -> None:
        with self.assertRaises(SystemExit):
            parse_android_args(["--preset", "debug", "--variants", "release"])


if __name__ == "__main__":
    unittest.main()
