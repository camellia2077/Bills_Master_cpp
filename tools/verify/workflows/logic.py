from __future__ import annotations

import argparse
from pathlib import Path

from .common import parse_forwarded_args, run


def run_logic_tests(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    args, passthrough = parse_forwarded_args(
        forwarded,
        lambda parser: parser.add_argument(
            "--skip-core-dist",
            action="store_true",
            help="Skip bills_core dist preparation before logic checks.",
        ),
    )

    if not args.skip_core_dist:
        core_build_entry = repo_root / "tools" / "flows" / "build_bills_tracer_core.py"
        build_code = run(
            [
                python_exe,
                str(core_build_entry),
                "--preset",
                "debug",
            ]
        )
        if build_code != 0:
            return build_code

    if passthrough:
        print("[ERROR] logic-tests no longer forwards additional commands.")
        return 2
    return 0


def run_module_mode_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    def configure_parser(parser: argparse.ArgumentParser) -> None:
        parser.add_argument(
            "--preset",
            default="debug",
            choices=["debug", "release"],
            help="Preset for bills_core dual-mode check.",
        )
        parser.add_argument(
            "--compiler",
            default="clang",
            choices=["clang"],
            help="Compiler for bills_core dual-mode check (clang only).",
        )

    args, passthrough = parse_forwarded_args(forwarded, configure_parser)

    core_build_entry = repo_root / "tools" / "flows" / "build_bills_tracer_core.py"
    base_cmd = [
        python_exe,
        str(core_build_entry),
        "--preset",
        args.preset,
        "--compiler",
        args.compiler,
    ]
    off_cmd = [*base_cmd, "--no-modules", *passthrough]
    on_cmd = [*base_cmd, "--modules", *passthrough]

    print("[INFO] Module mode check: BILLS_ENABLE_MODULES=OFF")
    off_code = run(off_cmd)
    if off_code != 0:
        print("[FAILED] Module mode check failed in OFF channel.")
        return off_code

    print("[INFO] Module mode check: BILLS_ENABLE_MODULES=ON")
    on_code = run(on_cmd)
    if on_code != 0:
        print("[FAILED] Module mode check failed in ON channel.")
        return on_code

    print("[OK] Module mode check passed for OFF/ON channels.")
    return 0


def run_tools_layer_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    entry = repo_root / "tools" / "verify" / "checks" / "check_tools_layering.py"
    return run([python_exe, str(entry), *forwarded])


def run_import_layer_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    entry = repo_root / "tools" / "verify" / "checks" / "check_import_layering.py"
    return run([python_exe, str(entry), *forwarded])


def run_boundary_layer_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    entry = repo_root / "tools" / "verify" / "checks" / "check_boundary_layering.py"
    return run([python_exe, str(entry), *forwarded])
