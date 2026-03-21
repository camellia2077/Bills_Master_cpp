from __future__ import annotations

import unittest
from pathlib import Path
from types import SimpleNamespace

from tools.flows.build_bills_android import parse_args as parse_android_args
from tools.flows.build_bills_android import resolve_requested_variants
from tools.toolchain.cli.main import parse_cli_args
from tools.toolchain.commands.build import run as run_build


class _FakeProcessRunner:
    def __init__(self) -> None:
        self.calls: list[tuple[list[str], Path]] = []

    def run(self, command: list[str], *, cwd: Path, check: bool = False) -> SimpleNamespace:
        self.calls.append((list(command), cwd))
        return SimpleNamespace(returncode=0, command=list(command), cwd=cwd, check=check)


class DistAndroidTests(unittest.TestCase):
    def test_dist_parser_accepts_android_target_and_forwards_unknown_args(self) -> None:
        _, args = parse_cli_args(["dist", "android", "--preset", "release", "--clean"])

        self.assertEqual(args.target, "android")
        self.assertEqual(args.preset, "release")
        self.assertEqual(args.scope, "shared")
        self.assertEqual(args.forwarded, ["--clean"])

    def test_run_build_routes_android_to_build_bills_android_flow(self) -> None:
        runner = _FakeProcessRunner()
        repo_root = Path("C:/repo")
        ctx = SimpleNamespace(
            python_executable="python",
            repo_root=repo_root,
            flow_entry=lambda relative_path: repo_root / "tools" / "flows" / relative_path,
            process_runner=runner,
        )
        args = SimpleNamespace(
            target="android",
            preset="release",
            scope="shared",
            forwarded=["--clean"],
        )

        exit_code = run_build(args, ctx)

        self.assertEqual(exit_code, 0)
        self.assertEqual(
            runner.calls,
            [
                (
                    [
                        "python",
                        str(repo_root / "tools" / "flows" / "build_bills_android.py"),
                        "--preset",
                        "release",
                        "--clean",
                    ],
                    repo_root,
                )
            ],
        )

    def test_run_build_rejects_tidy_for_android(self) -> None:
        runner = _FakeProcessRunner()
        ctx = SimpleNamespace(
            python_executable="python",
            repo_root=Path("C:/repo"),
            flow_entry=lambda relative_path: Path("C:/repo/tools/flows") / relative_path,
            process_runner=runner,
        )
        args = SimpleNamespace(target="android", preset="tidy", scope="shared", forwarded=[])

        exit_code = run_build(args, ctx)

        self.assertEqual(exit_code, 2)
        self.assertEqual(runner.calls, [])

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
