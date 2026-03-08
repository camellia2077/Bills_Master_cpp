from __future__ import annotations

import argparse
from pathlib import Path

from .common import run


def run_logic_tests(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--skip-core-dist",
        action="store_true",
        help="Skip bills_core dist preparation before ABI tests.",
    )
    parser.add_argument(
        "--skip-core-abi",
        action="store_true",
        help="Skip bills_core ABI tests.",
    )
    args, passthrough = parser.parse_known_args(forwarded)

    if args.skip_core_dist and args.skip_core_abi:
        print("[ERROR] logic-tests: both --skip-core-dist and --skip-core-abi are set.")
        return 2

    if not args.skip_core_dist:
        core_build_entry = repo_root / "tools" / "flows" / "build_bills_core.py"
        build_code = run(
            [
                python_exe,
                str(core_build_entry),
                "--preset",
                "debug",
                "--shared",
            ]
        )
        if build_code != 0:
            return build_code

    if not args.skip_core_abi:
        core_abi_entry = (
            repo_root / "tests" / "suites" / "logic" / "bills_core_abi" / "run_tests.py"
        )
        return run([python_exe, str(core_abi_entry), *passthrough])

    return 0


def run_module_mode_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--preset",
        default="debug",
        choices=["debug", "release"],
        help="Preset for bills_core dual-mode check.",
    )
    parser.add_argument(
        "--compiler",
        default="clang",
        choices=["clang", "gcc"],
        help="Compiler for bills_core dual-mode check.",
    )
    parser.add_argument(
        "--shared",
        action="store_true",
        default=True,
        help="Emit shared library into dist (default).",
    )
    parser.add_argument(
        "--static",
        dest="shared",
        action="store_false",
        help="Emit static library into dist.",
    )
    args, passthrough = parser.parse_known_args(forwarded)

    core_build_entry = repo_root / "tools" / "flows" / "build_bills_core.py"
    base_cmd = [
        python_exe,
        str(core_build_entry),
        "--preset",
        args.preset,
        "--compiler",
        args.compiler,
        "--shared" if args.shared else "--static",
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
    entry = repo_root / "tools" / "verify" / "tools" / "check_tools_layering.py"
    return run([python_exe, str(entry), *forwarded])


def run_import_layer_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    entry = repo_root / "tools" / "verify" / "tools" / "check_import_layering.py"
    return run([python_exe, str(entry), *forwarded])


def run_boundary_layer_check(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    entry = repo_root / "tools" / "verify" / "tools" / "check_boundary_layering.py"
    return run([python_exe, str(entry), *forwarded])
