#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[3]
SUMMARY_FILENAME = "test_summary.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def run_command(
    command: list[str],
    *,
    cwd: Path,
    expected_return_code: int,
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
    require(
        completed.returncode == expected_return_code,
        (
            f"Expected return code {expected_return_code}, got {completed.returncode}. "
            f"command={' '.join(command)}"
        ),
    )
    return completed


def resolve_generator_path(explicit_path: str) -> Path:
    if explicit_path.strip():
        path = Path(explicit_path).resolve()
        require(path.is_file(), f"generator executable not found: {path}")
        return path

    default_path = (
        REPO_ROOT
        / "tests"
        / "generators"
        / "log_generator"
        / "build_debug"
        / "bin"
        / "generator.exe"
    )
    require(default_path.is_file(), f"default generator executable not found: {default_path}")
    return default_path


def resolve_generator_config(explicit_path: str) -> Path:
    if explicit_path.strip():
        path = Path(explicit_path).resolve()
        require(path.is_file(), f"generator config not found: {path}")
        return path

    default_path = (
        REPO_ROOT
        / "tests"
        / "generators"
        / "log_generator"
        / "build_debug"
        / "bin"
        / "config"
        / "config.json"
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
        shutil.copy2(config_path, runtime_dir / "config.json")

        run_case(
            "help",
            lambda: require(
                "Usage:"
                in run_command(
                    [str(generator_path), "--help"],
                    cwd=runtime_dir,
                    expected_return_code=0,
                ).stderr,
                "help output must contain Usage.",
            ),
        )

        run_case(
            "version",
            lambda: require(
                "generator version"
                in run_command(
                    [str(generator_path), "--version"],
                    cwd=runtime_dir,
                    expected_return_code=0,
                ).stdout,
                "version output must contain generator version.",
            ),
        )

        run_case(
            "single_missing_year",
            lambda: require(
                "Missing argument for --single option."
                in run_command(
                    [str(generator_path), "--single"],
                    cwd=runtime_dir,
                    expected_return_code=1,
                ).stderr,
                "missing --single year should report explicit error.",
            ),
        )

        run_case(
            "single_invalid_year",
            lambda: require(
                "Invalid year provided for --single."
                in run_command(
                    [str(generator_path), "--single", "abc"],
                    cwd=runtime_dir,
                    expected_return_code=1,
                ).stderr,
                "invalid --single year should report explicit error.",
            ),
        )

        run_case(
            "double_invalid_range",
            lambda: require(
                "Start year cannot be after end year."
                in run_command(
                    [str(generator_path), "--double", "2025", "2024"],
                    cwd=runtime_dir,
                    expected_return_code=1,
                ).stderr,
                "invalid --double range should report explicit error.",
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
                read_first_line(
                    runtime_dir / "bills_output_from_config" / "2024" / "2024-01.txt"
                )
                == "date:2024-01",
                "single year output must use ISO date:YYYY-MM format.",
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
                read_first_line(
                    runtime_dir / "bills_output_from_config" / "2024" / "2024-01.txt"
                )
                == "date:2024-01",
                "double range output must use ISO date for 2024-01.",
            )
            require(
                read_first_line(
                    runtime_dir / "bills_output_from_config" / "2025" / "2025-12.txt"
                )
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
    parser = argparse.ArgumentParser(
        description="Run log_generator CLI command smoke tests."
    )
    parser.add_argument(
        "--generator",
        default="",
        help="Path to generator executable. Defaults to build_debug/bin/generator.exe.",
    )
    parser.add_argument(
        "--config",
        default="",
        help="Path to config.json. Defaults to build_debug/bin/config/config.json.",
    )
    parser.add_argument(
        "--summary",
        default=str(
            REPO_ROOT / "tests" / "output" / "logic" / "log_generator_cli" / SUMMARY_FILENAME
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
