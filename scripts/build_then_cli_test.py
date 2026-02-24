#!/usr/bin/env python3
"""Build apps/bills_master and then run CLI tests from test/run_command."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


PLUGIN_DLLS = [
    "md_month_formatter.dll",
    "rst_month_formatter.dll",
    "tex_month_formatter.dll",
    "typ_month_formatter.dll",
    "md_year_formatter.dll",
    "rst_year_formatter.dll",
    "tex_year_formatter.dll",
    "typ_year_formatter.dll",
]

CLEANUP_FILES = ["bills.sqlite3"]
CLEANUP_DIRS = [
    "txt_raw",
    "exported_files",
    "py_output",
    "build",
    "plugins",
    "config",
    "output",
]


def run_command(command: list[str], cwd: Path | None = None,
                env: dict[str, str] | None = None) -> None:
    print(f"==> {' '.join(command)}")
    subprocess.run(command, cwd=str(cwd) if cwd else None, env=env, check=True)


def parse_formats(raw_formats: str) -> list[str]:
    formats = [item.strip() for item in raw_formats.split(",") if item.strip()]
    if not formats:
        raise ValueError("No valid format found in --formats.")
    return formats


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(f"'{item}'" for item in items) + "]"


def write_temp_test_config(
    config_path: Path,
    build_bin_dir: Path,
    bills_dir: Path,
    import_dir: Path,
    run_export_all_tasks: bool,
    export_formats: list[str],
    ingest_mode: str,
    ingest_write_json: bool,
    single_year: str,
    single_month: str,
    range_start: str,
    range_end: str,
) -> None:
    config_text = f"""[paths]
build_dir = '{build_bin_dir.as_posix()}'
bills_dir = '{bills_dir.as_posix()}'
import_dir = '{import_dir.as_posix()}'

[run_control]
run_cleanup = true
run_prepare_env = true
run_tests = true

[plugins]
plugin_dlls = {to_toml_list(PLUGIN_DLLS)}

[settings]
run_export_all_tasks = {str(run_export_all_tasks).lower()}
ingest_mode = '{ingest_mode}'
ingest_write_json = {str(ingest_write_json).lower()}
export_formats = {to_toml_list(export_formats)}

[cleanup]
files_to_delete = {to_toml_list(CLEANUP_FILES)}
dirs_to_delete = {to_toml_list(CLEANUP_DIRS)}

[test_dates]
single_year = '{single_year}'
single_month = '{single_month}'
range_start = '{range_start}'
range_end = '{range_end}'
"""
    config_path.write_text(config_text, encoding="utf-8")


def build_cli(source_dir: Path, build_dir: Path, generator: str,
              build_type: str, target: str) -> None:
    configure_cmd = [
        "cmake",
        "-S",
        str(source_dir),
        "-B",
        str(build_dir),
        "-G",
        generator,
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]
    run_command(configure_cmd)

    build_cmd = ["cmake", "--build", str(build_dir), "--target", target]
    run_command(build_cmd)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build bill_master_cli and run command-line tests."
    )
    parser.add_argument("--build-dir", default="apps/bills_master/build_cli_test")
    parser.add_argument("--build-type", default="Debug")
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--target", default="bill_master_cli")
    parser.add_argument("--bills-dir", default="test/data/bills_output_from_config")
    parser.add_argument("--formats", default="md,tex,typ,rst")
    parser.add_argument("--ingest-mode", default="stepwise",
                        choices=["stepwise", "ingest"])
    parser.add_argument("--ingest-write-json", action="store_true")
    parser.add_argument("--run-export-all", action="store_true")
    parser.add_argument("--single-year", default="2024")
    parser.add_argument("--single-month", default="202401")
    parser.add_argument("--range-start", default="202401")
    parser.add_argument("--range-end", default="202412")
    parser.add_argument("--python", default=sys.executable)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    source_dir = repo_root / "apps" / "bills_master"
    build_dir = repo_root / args.build_dir
    build_bin_dir = build_dir / "bin"
    bills_dir = repo_root / args.bills_dir
    test_runner = repo_root / "test" / "run_command" / "run_tests.py"
    test_workdir = test_runner.parent
    import_dir = test_workdir / "output" / "txt2josn"

    if not bills_dir.exists():
        print(f"Error: bills data directory not found: {bills_dir}")
        print("Hint: generate data first (e.g. via apps/tools/log_generator).")
        return 2
    if not test_runner.exists():
        print(f"Error: test runner not found: {test_runner}")
        return 2

    export_formats = parse_formats(args.formats)

    try:
        build_cli(
            source_dir=source_dir,
            build_dir=build_dir,
            generator=args.generator,
            build_type=args.build_type,
            target=args.target,
        )
    except subprocess.CalledProcessError as exc:
        print(f"Build failed with exit code {exc.returncode}.")
        return exc.returncode

    temp_dir = repo_root / "temp"
    temp_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".toml",
        prefix="run_command_",
        dir=temp_dir,
        delete=False,
        encoding="utf-8",
    ) as tmp_file:
        temp_config_path = Path(tmp_file.name)

    try:
        write_temp_test_config(
            config_path=temp_config_path,
            build_bin_dir=build_bin_dir,
            bills_dir=bills_dir,
            import_dir=import_dir,
            run_export_all_tasks=args.run_export_all,
            export_formats=export_formats,
            ingest_mode=args.ingest_mode,
            ingest_write_json=args.ingest_write_json,
            single_year=args.single_year,
            single_month=args.single_month,
            range_start=args.range_start,
            range_end=args.range_end,
        )

        env = os.environ.copy()
        env["BILLS_MASTER_TEST_CONFIG"] = str(temp_config_path)
        run_command([args.python, str(test_runner)], cwd=test_workdir, env=env)
    except subprocess.CalledProcessError as exc:
        print(f"CLI tests failed with exit code {exc.returncode}.")
        return exc.returncode
    finally:
        temp_config_path.unlink(missing_ok=True)

    print("Build and CLI tests completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
