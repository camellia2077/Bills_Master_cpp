#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)

SUMMARY_FILENAME = "test_summary.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def run_command(
    command: list[str],
    *,
    cwd: Path,
    expected_return_code: int | None = None,
    expect_nonzero: bool = False,
) -> subprocess.CompletedProcess[str]:
    print(f"==> Running: {' '.join(command)}")
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )
    stdout = completed.stdout.strip()
    stderr = completed.stderr.strip()
    if stdout:
        print(stdout)
    if stderr:
        print(stderr)
    if expected_return_code is not None:
        require(
            completed.returncode == expected_return_code,
            (
                f"Expected return code {expected_return_code}, got {completed.returncode}. "
                f"command={' '.join(command)}"
            ),
        )
    if expect_nonzero:
        require(
            completed.returncode != 0,
            f"Expected a non-zero return code. command={' '.join(command)}",
        )
    return completed


def combined_output(completed: subprocess.CompletedProcess[str]) -> str:
    return "\n".join(part for part in [completed.stdout.strip(), completed.stderr.strip()] if part)


def contains_help_listing(text: str) -> bool:
    return "--single" in text and "--double" in text and "--version" in text


def resolve_generator_path(explicit_path: str) -> Path:
    from tools.toolchain.services.build_layout import resolve_build_directory

    if explicit_path.strip():
        path = Path(explicit_path).resolve()
        require(path.is_file(), f"generator executable not found: {path}")
        return path

    default_path = (
        resolve_build_directory(
            REPO_ROOT,
            target="bills-tracer-log-generator",
            preset="debug",
            scope="shared",
        ).build_dir
        / "bin"
        / "generator.exe"
    )
    require(default_path.is_file(), f"default generator executable not found: {default_path}")
    return default_path


def resolve_generator_config(explicit_path: str) -> Path:
    from tools.toolchain.services.build_layout import resolve_build_directory

    if explicit_path.strip():
        path = Path(explicit_path).resolve()
        require(path.is_file(), f"generator config not found: {path}")
        return path

    default_path = (
        resolve_build_directory(
            REPO_ROOT,
            target="bills-tracer-log-generator",
            preset="debug",
            scope="shared",
        ).build_dir
        / "bin"
        / "config"
        / "config.toml"
    )
    require(default_path.is_file(), f"default generator config not found: {default_path}")
    return default_path


def count_generated_txt(base_dir: Path, year: int) -> int:
    year_dir = base_dir / "bills_output_from_config" / str(year)
    if not year_dir.is_dir():
        return 0
    return len(list(year_dir.glob("*.txt")))


def read_first_line(file_path: Path) -> str:
    require(file_path.is_file(), f"generated file not found: {file_path}")
    with file_path.open("r", encoding="utf-8") as handle:
        return handle.readline().strip()


def read_text(file_path: Path) -> str:
    require(file_path.is_file(), f"generated file not found: {file_path}")
    return file_path.read_text(encoding="utf-8")


def extract_header_remark_lines(file_path: Path) -> list[str]:
    lines = read_text(file_path).splitlines()
    require(lines, f"generated file is empty: {file_path}")
    require(lines[0].startswith("date:"), f"first line must be date metadata: {file_path}")
    require(len(lines) > 1, f"remark metadata is missing: {file_path}")

    remark_lines: list[str] = []
    index = 1
    while index < len(lines) and lines[index].startswith("remark:"):
        remark_lines.append(lines[index][len("remark:"):])
        index += 1

    require(remark_lines, f"expected at least one remark metadata line in {file_path}")
    require(
        index < len(lines) and lines[index] == "",
        f"remark metadata block must be followed by a blank line in {file_path}",
    )
    return remark_lines


def run_cli_tests(generator_path: Path, config_path: Path) -> dict:
    total = 0
    passed = 0

    def run_case(name: str, fn) -> None:
        nonlocal total, passed
        total += 1
        fn()
        passed += 1
        print(f"[PASS] {name}")

    with tempfile.TemporaryDirectory(prefix="log_generator_cli_") as temp_dir:
        runtime_dir = Path(temp_dir)
        shutil.copy2(config_path, runtime_dir / "config.toml")

        run_case(
            "help",
            lambda: require(
                contains_help_listing(
                    combined_output(
                    run_command(
                        [str(generator_path), "--help"],
                        cwd=runtime_dir,
                        expected_return_code=0,
                    )
                    )
                ),
                "help output must list the CLI options.",
            ),
        )

        run_case(
            "version",
            lambda: require(
                "generator version"
                in combined_output(
                    run_command(
                    [str(generator_path), "--version"],
                    cwd=runtime_dir,
                    expected_return_code=0,
                    )
                ),
                "version output must contain generator version.",
            ),
        )

        run_case(
            "no_args",
            lambda: require(
                "Run with --help for more information."
                in combined_output(
                    run_command(
                        [str(generator_path)],
                        cwd=runtime_dir,
                        expect_nonzero=True,
                    )
                ),
                "running without arguments should point the user to --help.",
            ),
        )

        run_case(
            "single_missing_year",
            lambda: (
                lambda completed: (
                    require("--single" in combined_output(completed), "missing --single year should mention --single."),
                    require("missing" in combined_output(completed), "missing --single year should report a missing value."),
                )
            )(
                run_command(
                    [str(generator_path), "--single"],
                    cwd=runtime_dir,
                    expect_nonzero=True,
                )
            ),
        )

        run_case(
            "single_invalid_year",
            lambda: (
                lambda completed: (
                    require("--single" in combined_output(completed), "invalid --single year should mention --single."),
                    require("abc" in combined_output(completed), "invalid --single year should mention the bad value."),
                )
            )(
                run_command(
                    [str(generator_path), "--single", "abc"],
                    cwd=runtime_dir,
                    expect_nonzero=True,
                )
            ),
        )

        run_case(
            "double_invalid_range",
            lambda: require(
                "Start year cannot be after end year."
                in combined_output(
                    run_command(
                        [str(generator_path), "--double", "2025", "2024"],
                        cwd=runtime_dir,
                        expect_nonzero=True,
                    )
                ),
                "invalid --double range should report explicit error.",
            ),
        )

        run_case(
            "single_and_double_conflict",
            lambda: (
                lambda completed: (
                    require("--single" in combined_output(completed), "conflict output should mention --single."),
                    require("--double" in combined_output(completed), "conflict output should mention --double."),
                    require("excludes" in combined_output(completed), "conflict output should report the exclusion rule."),
                )
            )(
                run_command(
                    [str(generator_path), "--single", "2024", "--double", "2024", "2025"],
                    cwd=runtime_dir,
                    expect_nonzero=True,
                )
            ),
        )

        def case_single_generation() -> None:
            run_command(
                [str(generator_path), "--single", "2024"],
                cwd=runtime_dir,
                expected_return_code=0,
            )
            require(
                count_generated_txt(runtime_dir, 2024) == 12,
                "single year generation should create 12 monthly files.",
            )
            require(
                read_first_line(runtime_dir / "bills_output_from_config" / "2024" / "2024-01.txt")
                == "date:2024-01",
                "single year output must use ISO date:YYYY-MM format.",
            )
            january_remarks = extract_header_remark_lines(
                runtime_dir / "bills_output_from_config" / "2024" / "2024-01.txt"
            )
            february_remarks = extract_header_remark_lines(
                runtime_dir / "bills_output_from_config" / "2024" / "2024-02.txt"
            )
            march_remarks = extract_header_remark_lines(
                runtime_dir / "bills_output_from_config" / "2024" / "2024-03.txt"
            )
            require(
                len(january_remarks) == 2 and all(january_remarks),
                "2024-01 should contain a two-line non-empty remark block.",
            )
            require(
                february_remarks == [""],
                "2024-02 should contain an empty remark metadata line.",
            )
            require(
                len(march_remarks) == 1 and march_remarks[0] != "",
                "2024-03 should contain a single non-empty remark line.",
            )

        run_case("single_generation", case_single_generation)

        def case_double_generation() -> None:
            output_root = runtime_dir / "bills_output_from_config"
            if output_root.exists():
                shutil.rmtree(output_root)
            run_command(
                [str(generator_path), "--double", "2024", "2025"],
                cwd=runtime_dir,
                expected_return_code=0,
            )
            require(
                count_generated_txt(runtime_dir, 2024) == 12,
                "double range generation should create 12 files for 2024.",
            )
            require(
                count_generated_txt(runtime_dir, 2025) == 12,
                "double range generation should create 12 files for 2025.",
            )
            require(
                read_first_line(runtime_dir / "bills_output_from_config" / "2024" / "2024-01.txt")
                == "date:2024-01",
                "double range output must use ISO date for 2024-01.",
            )
            require(
                read_first_line(runtime_dir / "bills_output_from_config" / "2025" / "2025-12.txt")
                == "date:2025-12",
                "double range output must use ISO date for 2025-12.",
            )

        run_case("double_generation", case_double_generation)

    failed = total - passed
    return {
        "ok": failed == 0,
        "total": total,
        "success": passed,
        "failed": failed,
    }


def write_summary(summary_path: Path, payload: dict) -> None:
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def main() -> int:
    from tools.toolchain.services.build_layout import resolve_tests_root

    parser = argparse.ArgumentParser(description="Run log_generator CLI command smoke tests.")
    parser.add_argument(
        "--generator",
        default="",
        help="Path to generator executable. Defaults to dist/cmake/bills_tracer_log_generator/debug/shared/bin/generator.exe.",
    )
    parser.add_argument(
        "--config",
        default="",
        help="Path to config.toml. Defaults to dist/cmake/bills_tracer_log_generator/debug/shared/bin/config/config.toml.",
    )
    parser.add_argument(
        "--summary",
        default=str(
            resolve_tests_root(REPO_ROOT) / "logic" / "log_generator_cli" / SUMMARY_FILENAME
        ),
        help="Summary output path.",
    )
    args = parser.parse_args()

    summary_path = Path(args.summary).resolve()

    try:
        generator_path = resolve_generator_path(args.generator)
        config_path = resolve_generator_config(args.config)
        summary = run_cli_tests(generator_path, config_path)
        write_summary(summary_path, summary)
    except Exception as exc:  # pylint: disable=broad-except
        failure_summary = {
            "ok": False,
            "total": 0,
            "success": 0,
            "failed": 1,
            "note": str(exc),
        }
        write_summary(summary_path, failure_summary)
        print(f"[FAILED] {exc}")
        return 1

    print(f"[OK] log_generator CLI tests passed. summary={summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
