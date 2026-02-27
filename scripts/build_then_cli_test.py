#!/usr/bin/env python3
"""Build apps/bills_windows_cli and then run CLI tests from test/."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path


FORMAT_CONFIG = {
    "md": {
        "cmake_option": "ENABLE_FMT_MD",
        "targets": ["md_month_formatter", "md_year_formatter"],
        "dlls": ["md_month_formatter.dll", "md_year_formatter.dll"],
    },
    "rst": {
        "cmake_option": "ENABLE_FMT_RST",
        "targets": ["rst_month_formatter", "rst_year_formatter"],
        "dlls": ["rst_month_formatter.dll", "rst_year_formatter.dll"],
    },
    "tex": {
        "cmake_option": "ENABLE_FMT_TEX",
        "targets": ["tex_month_formatter", "tex_year_formatter"],
        "dlls": ["tex_month_formatter.dll", "tex_year_formatter.dll"],
    },
    "typ": {
        "cmake_option": "ENABLE_FMT_TYP",
        "targets": ["typ_month_formatter", "typ_year_formatter"],
        "dlls": ["typ_month_formatter.dll", "typ_year_formatter.dll"],
    },
}
RUNTIME_EXPORT_FORMATS_FILENAME = "Export_Formats.json"
TEST_SUMMARY_FILENAME = "test_summary.json"
PYTHON_TEST_LOG_FILENAME = "test_python_output.log"

CLEANUP_FILES = ["bills.sqlite3"]
CLEANUP_DIRS = [
    "output",
    "build",
    "plugins",
    "config",
]


def run_command(command: list[str], cwd: Path | None = None,
                env: dict[str, str] | None = None) -> None:
    print(f"==> {' '.join(command)}")
    subprocess.run(command, cwd=str(cwd) if cwd else None, env=env, check=True)


def load_test_summary(summary_path: Path) -> dict | None:
    if not summary_path.exists():
        return None
    try:
        return json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def write_python_test_log(
    log_path: Path,
    command: list[str],
    started_at: datetime,
    completed_at: datetime,
    return_code: int,
    stdout: str,
    stderr: str,
) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        f"started_at={started_at.isoformat(timespec='seconds')}",
        f"completed_at={completed_at.isoformat(timespec='seconds')}",
        f"return_code={return_code}",
        f"command={' '.join(command)}",
        "",
        "stdout:",
        stdout.rstrip(),
        "",
        "stderr:",
        stderr.rstrip(),
        "",
    ]
    log_path.write_text("\n".join(lines), encoding="utf-8")


def read_cache_home_directory(cache_file: Path) -> Path | None:
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
                    return Path(line.split("=", 1)[1].strip())
    except OSError:
        return None
    return None


def prepare_build_dir(build_dir: Path, source_dir: Path) -> None:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return
    cached_home = read_cache_home_directory(cache_file)
    if cached_home is not None and cached_home.resolve() == source_dir.resolve():
        return
    shutil.rmtree(build_dir)


def parse_formats(raw_formats: str) -> list[str]:
    formats = [item.strip().lower() for item in raw_formats.split(",") if item.strip()]
    if not formats:
        raise ValueError("No valid format found in --formats.")
    unsupported = [fmt for fmt in formats if fmt not in FORMAT_CONFIG]
    if unsupported:
        raise ValueError(
            f"Unsupported format(s): {', '.join(unsupported)}. "
            f"Supported: {', '.join(FORMAT_CONFIG.keys())}."
        )
    deduped: list[str] = []
    for fmt in formats:
        if fmt not in deduped:
            deduped.append(fmt)
    return deduped


def selected_plugin_targets(formats: list[str]) -> list[str]:
    targets: list[str] = []
    for fmt in formats:
        targets.extend(FORMAT_CONFIG[fmt]["targets"])
    return targets


def selected_plugin_dlls(formats: list[str]) -> list[str]:
    dlls: list[str] = []
    for fmt in formats:
        dlls.extend(FORMAT_CONFIG[fmt]["dlls"])
    return dlls


def cmake_format_defines(formats: list[str]) -> list[str]:
    enabled = set(formats)
    defines: list[str] = []
    for fmt, cfg in FORMAT_CONFIG.items():
        option = cfg["cmake_option"]
        defines.append(f"-D{option}={'ON' if fmt in enabled else 'OFF'}")
    return defines


def write_runtime_export_formats_config(
    build_bin_dir: Path, export_formats: list[str]
) -> None:
    config_dir = build_bin_dir / "config"
    config_dir.mkdir(parents=True, exist_ok=True)
    config_path = config_dir / RUNTIME_EXPORT_FORMATS_FILENAME
    config_payload = {"enabled_formats": export_formats}
    config_path.write_text(
        json.dumps(config_payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(f"'{item}'" for item in items) + "]"


def write_temp_test_config(
    config_path: Path,
    build_bin_dir: Path,
    bills_dir: Path,
    import_dir: Path,
    plugin_dlls: list[str],
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
plugin_dlls = {to_toml_list(plugin_dlls)}

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
              build_type: str, target: str,
              plugin_targets: list[str], cmake_defines: list[str]) -> None:
    prepare_build_dir(build_dir, source_dir)
    configure_cmd = [
        "cmake",
        "-S",
        str(source_dir),
        "-B",
        str(build_dir),
        "-G",
        generator,
        f"-DCMAKE_BUILD_TYPE={build_type}",
        *cmake_defines,
    ]
    run_command(configure_cmd)

    build_targets = [target, *plugin_targets]
    build_cmd = ["cmake", "--build", str(build_dir), "--target", *build_targets]
    run_command(build_cmd)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build bill_master_cli and run command-line tests."
    )
    parser.add_argument("--build-dir", default="apps/bills_windows_cli/build_cli_test")
    parser.add_argument("--build-type", default="Debug")
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--target", default="bill_master_cli")
    parser.add_argument("--bills-dir", default="test/data/bills_output_from_config")
    parser.add_argument("--formats", default="md")
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
    source_dir = repo_root / "apps" / "bills_windows_cli"
    build_dir = repo_root / args.build_dir
    build_bin_dir = build_dir / "bin"
    bills_dir = repo_root / args.bills_dir
    test_root = repo_root / "test"
    test_runner = test_root / "suites" / "bills_master" / "run_tests.py"
    test_workdir = test_root
    import_dir = test_root / "output" / "txt2josn"
    summary_path = test_root / "output" / TEST_SUMMARY_FILENAME
    python_test_log_path = test_root / "output" / PYTHON_TEST_LOG_FILENAME

    if not bills_dir.exists():
        print(f"Error: bills data directory not found: {bills_dir}")
        print("Hint: generate data first (e.g. via apps/tools/log_generator).")
        return 2
    if not test_runner.exists():
        print(f"Error: test runner not found: {test_runner}")
        return 2

    export_formats = parse_formats(args.formats)
    plugin_targets = selected_plugin_targets(export_formats)
    plugin_dlls = selected_plugin_dlls(export_formats)
    format_defines = cmake_format_defines(export_formats)

    try:
        build_cli(
            source_dir=source_dir,
            build_dir=build_dir,
            generator=args.generator,
            build_type=args.build_type,
            target=args.target,
            plugin_targets=plugin_targets,
            cmake_defines=format_defines,
        )
        write_runtime_export_formats_config(build_bin_dir, export_formats)
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
            plugin_dlls=plugin_dlls,
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
        print(f"==> {args.python} {test_runner}")
        test_cmd = [args.python, str(test_runner)]
        started_at = datetime.now()
        test_result = subprocess.run(
            test_cmd,
            cwd=str(test_workdir),
            env=env,
            capture_output=True,
            text=True,
            check=False,
        )
        completed_at = datetime.now()
        print(test_result.stdout, end="")
        if test_result.stderr:
            print(test_result.stderr, end="", file=sys.stderr)
        write_python_test_log(
            log_path=python_test_log_path,
            command=test_cmd,
            started_at=started_at,
            completed_at=completed_at,
            return_code=test_result.returncode,
            stdout=test_result.stdout,
            stderr=test_result.stderr,
        )
        print(f"CLI python output log: {python_test_log_path}")
        summary = load_test_summary(summary_path)
        if summary is None:
            print(f"CLI tests did not produce summary JSON: {summary_path}")
            return test_result.returncode if test_result.returncode != 0 else 3
        success = int(summary.get("success", 0))
        failed = int(summary.get("failed", 0))
        total = int(summary.get("total", 0))
        ok = bool(summary.get("ok", False))
        print(
            "CLI test summary: "
            f"ok={ok}, total={total}, success={success}, failed={failed}"
        )
        if not ok or failed > 0:
            return test_result.returncode if test_result.returncode != 0 else 1
    except subprocess.CalledProcessError as exc:
        print(f"CLI tests failed with exit code {exc.returncode}.")
        return exc.returncode
    finally:
        temp_config_path.unlink(missing_ok=True)

    print("Build and CLI tests completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
