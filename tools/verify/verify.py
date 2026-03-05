#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from statistics import mean, median


def normalize_extra(extra_args: list[str]) -> list[str]:
    if extra_args and extra_args[0] == "--":
        return extra_args[1:]
    return extra_args


def run(command: list[str]) -> int:
    print(f"==> Running: {' '.join(command)}")
    return subprocess.call(command)


def resolve_project_output_root(
    repo_root: Path,
    project: str,
    preferred_group: str = "artifact",
) -> Path:
    return repo_root / "tests" / "output" / preferred_group / project


def load_test_summary(repo_root: Path, project: str) -> dict | None:
    summary_path = (
        resolve_project_output_root(repo_root, project) / "test_summary.json"
    )
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
    log_path = (
        resolve_project_output_root(repo_root, project) / "test_python_output.log"
    )
    if not log_path.exists():
        print(f"[ERROR] Missing python test log: {log_path}")
        return None

    started_at_raw = ""
    completed_at_raw = ""
    elapsed_raw = ""
    try:
        for line in log_path.read_text(encoding="utf-8").splitlines():
            if line.startswith("started_at="):
                started_at_raw = line.split("=", 1)[1].strip()
            elif line.startswith("completed_at="):
                completed_at_raw = line.split("=", 1)[1].strip()
            elif line.startswith("elapsed_seconds="):
                elapsed_raw = line.split("=", 1)[1].strip()
            if elapsed_raw or (started_at_raw and completed_at_raw):
                break
    except OSError as exc:
        print(f"[ERROR] Unable to read python test log: {log_path} ({exc})")
        return None

    if elapsed_raw:
        try:
            elapsed = float(elapsed_raw)
        except ValueError as exc:
            print(
                f"[ERROR] Invalid elapsed_seconds in python test log: "
                f"{log_path} ({exc})"
            )
            return None
        if elapsed < 0:
            print(f"[ERROR] Negative elapsed_seconds in python test log: {log_path}")
            return None
        return elapsed

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
    model_projects: list[str],
    json_projects: list[str],
    max_regression_ratio: float,
    stat_method: str,
) -> int:
    model_durations: list[float] = []
    json_durations: list[float] = []

    for project in model_projects:
        duration = load_python_test_duration_seconds(repo_root, project)
        if duration is None:
            return 2
        model_durations.append(duration)

    for project in json_projects:
        duration = load_python_test_duration_seconds(repo_root, project)
        if duration is None:
            return 2
        json_durations.append(duration)

    if not model_durations or not json_durations:
        print("[ERROR] Empty duration samples for performance gate.")
        return 2

    if any(duration <= 0 for duration in json_durations):
        print(
            "[WARN] Skip performance gate because json-first duration "
            f"is not positive: {json_durations}"
        )
        return 0

    if stat_method == "mean":
        model_duration = mean(model_durations)
        json_duration = mean(json_durations)
    else:
        model_duration = median(model_durations)
        json_duration = median(json_durations)

    ratio = (model_duration - json_duration) / json_duration
    model_samples = ", ".join(f"{duration:.3f}s" for duration in model_durations)
    json_samples = ", ".join(f"{duration:.3f}s" for duration in json_durations)
    print(
        "[INFO] Performance check: "
        f"stat={stat_method}, "
        f"model-samples=[{model_samples}], "
        f"json-samples=[{json_samples}], "
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
    parser.add_argument(
        "--performance-stat",
        default="median",
        choices=["median", "mean"],
        help="Aggregation method for dual-order performance samples.",
    )
    args, passthrough = parser.parse_known_args(forwarded)
    if args.max_performance_regression < 0:
        print("[ERROR] --max-performance-regression must be >= 0.")
        return 2

    build_then_entry = repo_root / "tools" / "build" / "build_then_cli_test.py"
    compare_entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"

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
    model_rev_project = f"{args.model_project}__rev"
    json_rev_project = f"{args.json_project}__rev"
    model_cmd_rev = [
        python_exe,
        str(build_then_entry),
        "--export-pipeline",
        "model-first",
        "--output-project",
        model_rev_project,
        "--run-tag",
        "parallel_model_rev",
        "--formats",
        args.formats,
        "--build-dir-mode",
        args.build_dir_mode,
        *passthrough,
    ]
    json_cmd_rev = [
        python_exe,
        str(build_then_entry),
        "--export-pipeline",
        "json-first",
        "--output-project",
        json_rev_project,
        "--run-tag",
        "parallel_json_rev",
        "--formats",
        args.formats,
        "--build-dir-mode",
        args.build_dir_mode,
        *passthrough,
    ]

    if args.enforce_performance_gate:
        # Performance gate should avoid cross-run contention (build/test overlap),
        # otherwise one pipeline may be penalized by the other pipeline's build load.
        # Also run both execution orders to reduce first-run cold-start bias.
        print(f"==> Running (perf gate order A model->json): {' '.join(model_cmd)}")
        model_code = subprocess.call(model_cmd)
        print(f"==> Running (perf gate order A model->json): {' '.join(json_cmd)}")
        json_code = subprocess.call(json_cmd)
        print(f"==> Running (perf gate order B json->model): {' '.join(json_cmd_rev)}")
        json_rev_code = subprocess.call(json_cmd_rev)
        print(f"==> Running (perf gate order B json->model): {' '.join(model_cmd_rev)}")
        model_rev_code = subprocess.call(model_cmd_rev)
    else:
        print(f"==> Running (parallel): {' '.join(model_cmd)}")
        model_proc = subprocess.Popen(model_cmd)
        print(f"==> Running (parallel): {' '.join(json_cmd)}")
        json_proc = subprocess.Popen(json_cmd)

        model_code = model_proc.wait()
        json_code = json_proc.wait()
        model_rev_code = 0
        json_rev_code = 0

    if (
        model_code != 0
        or json_code != 0
        or model_rev_code != 0
        or json_rev_code != 0
    ):
        print(
            "[FAILED] Parallel bills workflow failed: "
            f"model_exit={model_code}, json_exit={json_code}, "
            f"model_rev_exit={model_rev_code}, json_rev_exit={json_rev_code}"
        )
        for code in (model_code, json_code, model_rev_code, json_rev_code):
            if code != 0:
                return code
        return 1

    model_summary_code = validate_test_summary(repo_root, args.model_project)
    json_summary_code = validate_test_summary(repo_root, args.json_project)
    if model_summary_code != 0:
        return model_summary_code
    if json_summary_code != 0:
        return json_summary_code
    if args.enforce_performance_gate:
        model_rev_summary_code = validate_test_summary(repo_root, model_rev_project)
        json_rev_summary_code = validate_test_summary(repo_root, json_rev_project)
        if model_rev_summary_code != 0:
            return model_rev_summary_code
        if json_rev_summary_code != 0:
            return json_rev_summary_code

    compare_cmd = [
        python_exe,
        str(compare_entry),
        "--output-group",
        "artifact",
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
        model_projects=[args.model_project, model_rev_project],
        json_projects=[args.json_project, json_rev_project],
        max_regression_ratio=args.max_performance_regression,
        stat_method=args.performance_stat,
    )


def run_logic_tests(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--skip-core-build",
        action="store_true",
        help="Skip bills_core build before ABI tests.",
    )
    parser.add_argument(
        "--skip-core-abi",
        action="store_true",
        help="Skip bills_core ABI tests.",
    )
    args, passthrough = parser.parse_known_args(forwarded)

    if args.skip_core_build and args.skip_core_abi:
        print("[ERROR] logic-tests: both --skip-core-build and --skip-core-abi are set.")
        return 2

    if not args.skip_core_build:
        core_build_entry = repo_root / "tools" / "build" / "build_bills_core.py"
        build_code = run([python_exe, str(core_build_entry), "build_fast", "--shared"])
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
        "--command",
        default="build_fast",
        choices=["build", "build_fast"],
        help="Build command for bills_core dual-mode check.",
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
        help="Build shared library (default).",
    )
    parser.add_argument(
        "--static",
        dest="shared",
        action="store_false",
        help="Build static library.",
    )
    args, passthrough = parser.parse_known_args(forwarded)

    core_build_entry = repo_root / "tools" / "build" / "build_bills_core.py"
    base_cmd = [
        python_exe,
        str(core_build_entry),
        args.command,
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


def run_artifact_tests(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--scope",
        default="full",
        choices=["basic", "smoke", "gate", "full"],
        help="basic=bills, smoke=bills-parallel-smoke, gate=report-consistency-gate, full=basic+gate",
    )
    parser.add_argument(
        "--enforce-performance-gate",
        action="store_true",
        help="Enable performance regression gate in gate/full scope.",
    )
    args, passthrough = parser.parse_known_args(forwarded)

    if args.scope in ("basic", "full"):
        bills_entry = repo_root / "tools" / "build" / "build_then_cli_test.py"
        bills_code = run([python_exe, str(bills_entry)])
        if bills_code != 0:
            return bills_code

    if args.scope == "smoke":
        return run_bills_parallel_smoke(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=passthrough,
        )

    if args.scope in ("gate", "full"):
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
        ]
        if args.enforce_performance_gate:
            default_gate_args.extend(
                [
                    "--enforce-performance-gate",
                    "--max-performance-regression",
                    "0.10",
                    "--performance-stat",
                    "median",
                ]
            )
        return run_bills_parallel_smoke(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[*default_gate_args, *passthrough],
        )

    return 0


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
            "logic-tests",
            "artifact-tests",
            "all-tests",
            "module-mode-check",
            "tools-layer-check",
            "import-layer-check",
            "boundary-layer-check",
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
            "bills=build+CLI test, logic-tests=unit/component style checks, "
            "artifact-tests=integration/e2e/snapshot style checks, all-tests=logic+artifact, "
            "module-mode-check=dual-channel core build with BILLS_ENABLE_MODULES=OFF/ON, "
            "tools-layer-check=check tools/* layering dependency rules, "
            "import-layer-check=check call-layer include/import policy for C++ sources, "
            "boundary-layer-check=check boundary-layer include allowlist policy for C++ sources, "
            "bills-build=compile bills_cli, "
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

    repo_root = Path(__file__).resolve().parents[2]
    python_exe = sys.executable
    forwarded = normalize_extra(args.extra)

    if args.workflow == "bills":
        entry = repo_root / "tools" / "build" / "build_then_cli_test.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "logic-tests":
        return run_logic_tests(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "artifact-tests":
        return run_artifact_tests(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "all-tests":
        logic_code = run_logic_tests(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
        if logic_code != 0:
            return logic_code
        module_mode_code = run_module_mode_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
        if module_mode_code != 0:
            return module_mode_code
        tools_layer_code = run_tools_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
        if tools_layer_code != 0:
            return tools_layer_code
        import_layer_code = run_import_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
        if import_layer_code != 0:
            return import_layer_code
        boundary_layer_code = run_boundary_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
        if boundary_layer_code != 0:
            return boundary_layer_code
        return run_artifact_tests(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "module-mode-check":
        return run_module_mode_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "tools-layer-check":
        return run_tools_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "import-layer-check":
        return run_import_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "boundary-layer-check":
        return run_boundary_layer_check(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

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
            "--performance-stat",
            "median",
        ]
        return run_bills_parallel_smoke(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[*default_gate_args, *forwarded],
        )

    if args.workflow == "bills-build":
        entry = repo_root / "tools" / "build" / "build_bills_master.py"
        if not forwarded:
            forwarded = ["build_fast"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-build":
        entry = repo_root / "tools" / "build" / "build_bills_core.py"
        if not forwarded:
            forwarded = ["build_fast", "--shared"]
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-abi":
        entry = repo_root / "tests" / "suites" / "logic" / "bills_core_abi" / "run_tests.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "report-snapshot":
        entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "report-pipeline-compare":
        entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
        if not forwarded:
            forwarded = [
                "--compare-projects",
                "bills_tracer_model_first",
                "bills_tracer_json_first",
                "--compare-scope",
                "md",
            ]
        return run([python_exe, str(entry), *forwarded])

    entry = repo_root / "tools" / "build" / "build_log_generator.py"
    if not forwarded:
        forwarded = ["build", "--mode", "Debug"]
    return run([python_exe, str(entry), *forwarded])


if __name__ == "__main__":
    raise SystemExit(main())

