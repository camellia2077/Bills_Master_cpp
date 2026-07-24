from __future__ import annotations

import argparse
import os
from pathlib import Path

from ..services.build_layout import resolve_build_directory
from ..services.dist.core import run_core_dist

from .common import parse_forwarded_args, run


def _run_core_contract_tests(repo_root: Path, *, preset: str = "debug") -> int:
    spec = resolve_build_directory(
        repo_root,
        target="bills-tracer-core",
        preset=preset,
        scope="shared",
    )
    build_code = run(
        [
            "cmake",
            "--build",
            str(spec.build_dir),
            "--target",
            "bills_core_content_line_contract_tests",
        ]
    )
    if build_code != 0:
        return build_code

    executable_name = (
        "bills_core_content_line_contract_tests.exe"
        if os.name == "nt"
        else "bills_core_content_line_contract_tests"
    )
    return run([str(spec.build_dir / "bin" / executable_name)])


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
        try:
            build_code = run_core_dist(
                repo_root,
                preset="debug",
            )
        except SystemExit as exc:
            return int(exc.code) if isinstance(exc.code, int) else 1
        if build_code != 0:
            return build_code

    if passthrough:
        print("[ERROR] logic-tests no longer forwards additional commands.")
        return 2
    return _run_core_contract_tests(repo_root)


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

    print("[INFO] Module mode check: BILLS_ENABLE_MODULES=OFF")
    try:
        off_code = run_core_dist(
            repo_root,
            preset=args.preset,
            compiler=args.compiler,
            modules_enabled=False,
            extra_args=passthrough,
        )
    except SystemExit as exc:
        off_code = int(exc.code) if isinstance(exc.code, int) else 1
    if off_code != 0:
        print("[FAILED] Module mode check failed in OFF channel.")
        return off_code

    print("[INFO] Module mode check: BILLS_ENABLE_MODULES=ON")
    try:
        on_code = run_core_dist(
            repo_root,
            preset=args.preset,
            compiler=args.compiler,
            modules_enabled=True,
            extra_args=passthrough,
        )
    except SystemExit as exc:
        on_code = int(exc.code) if isinstance(exc.code, int) else 1
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
