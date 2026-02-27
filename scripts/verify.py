#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def normalize_extra(extra_args: list[str]) -> list[str]:
    if extra_args and extra_args[0] == "--":
        return extra_args[1:]
    return extra_args


def run(command: list[str]) -> int:
    print(f"==> Running: {' '.join(command)}")
    return subprocess.call(command)


def load_test_summary(repo_root: Path, project: str) -> dict | None:
    summary_path = repo_root / "test" / "output" / project / "test_summary.json"
    if not summary_path.exists():
        print(f"[ERROR] Missing test summary: {summary_path}")
        return None
    try:
        return json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"[ERROR] Invalid test summary JSON: {summary_path} ({exc})")
        return None


def validate_test_summary(repo_root: Path, project: str) -> int:
    summary = load_test_summary(repo_root, project)
    if summary is None:
        return 2
    ok = bool(summary.get("ok", False))
    failed = int(summary.get("failed", 0))
    if not ok or failed > 0:
        print(
            "[FAILED] Test summary validation failed: "
            f"project={project}, ok={ok}, failed={failed}"
        )
        return 1
    print(
        "[OK] Test summary validation passed: "
        f"project={project}, total={summary.get('total', 0)}, "
        f"success={summary.get('success', 0)}, failed={failed}"
    )
    return 0


def load_python_test_duration_seconds(repo_root: Path, project: str) -> float | None:
    log_path = repo_root / "test" / "output" / project / "test_python_output.log"
    if not log_path.exists():
        print(f"[ERROR] Missing python test log: {log_path}")
        return None

    started_at_raw = ""
    completed_at_raw = ""
    try:
        for line in log_path.read_text(encoding="utf-8").splitlines():
            if line.startswith("started_at="):
                started_at_raw = line.split("=", 1)[1].strip()
            elif line.startswith("completed_at="):
                completed_at_raw = line.split("=", 1)[1].strip()
            if started_at_raw and completed_at_raw:
                break
    except OSError as exc:
        print(f"[ERROR] Unable to read python test log: {log_path} ({exc})")
        return None

    if not started_at_raw or not completed_at_raw:
        print(f"[ERROR] Missing started_at/completed_at in python test log: {log_path}")
        return None

    try:
        started_at = datetime.fromisoformat(started_at_raw)
        completed_at = datetime.fromisoformat(completed_at_raw)
    except ValueError as exc:
        print(f"[ERROR] Invalid timestamp in python test log: {log_path} ({exc})")
        return None

    duration = (completed_at - started_at).total_seconds()
    if duration < 0:
        print(f"[ERROR] Negative duration from python test log: {log_path}")
        return None
    return duration


def validate_performance_regression(
    repo_root: Path,
    model_project: str,
    json_project: str,
    max_regression_ratio: float,
) -> int:
    model_duration = load_python_test_duration_seconds(repo_root, model_project)
    json_duration = load_python_test_duration_seconds(repo_root, json_project)
    if model_duration is None or json_duration is None:
        return 2

    if json_duration <= 0:
        print(
            "[WARN] Skip performance gate because json-first duration "
            f"is not positive: {json_duration:.3f}s"
        )
        return 0

    ratio = (model_duration - json_duration) / json_duration
    print(
        "[INFO] Performance check: "
        f"model-first={model_duration:.3f}s, "
        f"json-first={json_duration:.3f}s, "
        f"regression={ratio * 100:.2f}%, "
        f"threshold={max_regression_ratio * 100:.2f}%"
    )
    if ratio > max_regression_ratio:
        print(
            "[FAILED] Performance regression gate failed: "
            f"model-first regression {ratio * 100:.2f}% exceeds "
            f"threshold {max_regression_ratio * 100:.2f}%."
        )
        return 1
    print("[OK] Performance regression gate passed.")
    return 0


def run_bills_parallel_smoke(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--model-project",
        default="bills_tracer_model_first_parallel",
    )
    parser.add_argument(
        "--json-project",
        default="bills_tracer_json_first_parallel",
    )
    parser.add_argument(
        "--formats",
        default="md",
    )
    parser.add_argument(
        "--compare-scope",
        default="md",
        choices=["all", "md", "json", "tex", "typ"],
    )
    parser.add_argument(
        "--build-dir-mode",
        default="isolated",
        choices=["isolated", "shared"],
    )
    parser.add_argument(
        "--enforce-performance-gate",
        action="store_true",
        help="Fail if model-first runtime regression exceeds threshold vs json-first.",
    )
    parser.add_argument(
        "--max-performance-regression",
        type=float,
        default=0.10,
        help="Allowed model-first regression ratio vs json-first (default: 0.10 = +10%%).",
    )
    args, passthrough = parser.parse_known_args(forwarded)
    if args.max_performance_regression < 0:
        print("[ERROR] --max-performance-regression must be >= 0.")
        return 2

    build_then_entry = repo_root / "scripts" / "build_then_cli_test.py"
    compare_entry = repo_root / "scripts" / "check_report_snapshots.py"

    model_cmd = [
        python_exe,
        str(build_then_entry),
        "--export-pipeline",
        "model-first",
        "--output-project",
        args.model_project,
        "--run-tag",
        "parallel_model",
        "--formats",
        args.formats,
        "--build-dir-mode",
        args.build_dir_mode,
        *passthrough,
    ]
    json_cmd = [
        python_exe,
        str(build_then_entry),
        "--export-pipeline",
        "json-first",
        "--output-project",
        args.json_project,
        "--run-tag",
        "parallel_json",
        "--formats",
        args.formats,
        "--build-dir-mode",
        args.build_dir_mode,
        *passthrough,
    ]

    print(f"==> Running (parallel): {' '.join(model_cmd)}")
    model_proc = subprocess.Popen(model_cmd)
    print(f"==> Running (parallel): {' '.join(json_cmd)}")
    json_proc = subprocess.Popen(json_cmd)

    model_code = model_proc.wait()
    json_code = json_proc.wait()

    if model_code != 0 or json_code != 0:
        print(
            "[FAILED] Parallel bills workflow failed: "
            f"model_exit={model_code}, json_exit={json_code}"
        )
        return model_code if model_code != 0 else json_code

    model_summary_code = validate_test_summary(repo_root, args.model_project)
    json_summary_code = validate_test_summary(repo_root, args.json_project)
    if model_summary_code != 0:
        return model_summary_code
    if json_summary_code != 0:
        return json_summary_code

    compare_cmd = [
        python_exe,
        str(compare_entry),
        "--compare-projects",
        args.model_project,
        args.json_project,
        "--compare-scope",
        args.compare_scope,
    ]
    compare_code = run(compare_cmd)
    if compare_code != 0:
        return compare_code

    if not args.enforce_performance_gate:
        return 0

    return validate_performance_regression(
        repo_root=repo_root,
        model_project=args.model_project,
        json_project=args.json_project,
        max_regression_ratio=args.max_performance_regression,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Unified verify entry for build and test workflows."
    )
    parser.add_argument(
        "workflow",
        nargs="?",
        default="bills",
        choices=[
            "bills",
            "bills-parallel-smoke",
            "report-consistency-gate",
            "bills-build",
            "core-build",
            "core-abi",
            "log-build",
            "report-snapshot",
            "report-pipeline-compare",
        ],
        help=(
            "bills=build+CLI test, bills-build=compile bills_windows_cli, "
            "bills-parallel-smoke=run model-first/json-first bills in parallel and compare outputs, "
            "report-consistency-gate=run full model/json consistency gate with performance threshold, "
            "core-build=compile bills_core, core-abi=run ABI smoke tests, "
            "log-build=compile log_generator, "
            "report-snapshot=compare exported reports with frozen snapshots, "
            "report-pipeline-compare=compare model-first and json-first outputs"
        ),
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Forwarded arguments for the selected workflow",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    python_exe = sys.executable
    forwarded = normalize_extra(args.extra)

    if args.workflow == "bills":
        entry = repo_root / "scripts" / "build_then_cli_test.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "bills-parallel-smoke":
        return run_bills_parallel_smoke(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "report-consistency-gate":
        default_gate_args = [
            "--model-project",
            "bills_gate_model_first",
            "--json-project",
            "bills_gate_json_first",
            "--formats",
            "md,tex,typ",
            "--compare-scope",
            "all",
            "--build-dir-mode",
            "isolated",
            "--enforce-performance-gate",
            "--max-performance-regression",
            "0.10",
        ]
        return run_bills_parallel_smoke(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[*default_gate_args, *forwarded],
        )

    if args.workflow == "bills-build":
        entry = repo_root / "scripts" / "build_bills_master.py"
        if not forwarded:
            forwarded = ["build_fast"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-build":
        entry = repo_root / "scripts" / "build_bills_core.py"
        if not forwarded:
            forwarded = ["build_fast", "--shared"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-abi":
        entry = repo_root / "scripts" / "test_bills_core_abi.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "report-snapshot":
        entry = repo_root / "scripts" / "check_report_snapshots.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "report-pipeline-compare":
        entry = repo_root / "scripts" / "check_report_snapshots.py"
        if not forwarded:
            forwarded = [
                "--compare-projects",
                "bills_tracer_model_first",
                "bills_tracer_json_first",
                "--compare-scope",
                "md",
            ]
        return run([python_exe, str(entry), *forwarded])

    entry = repo_root / "scripts" / "build_log_generator.py"
    if not forwarded:
        forwarded = ["build", "--mode", "Debug"]
    return run([python_exe, str(entry), *forwarded])


if __name__ == "__main__":
    raise SystemExit(main())
