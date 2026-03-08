#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from statistics import mean, median

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    assert_no_legacy_flags,
    resolve_artifact_latest_dir,
    resolve_artifact_project_root,
    resolve_logic_pipeline_root,
    sanitize_segment,
)


def normalize_extra(extra_args: list[str]) -> list[str]:
    if extra_args and extra_args[0] == "--":
        return extra_args[1:]
    return extra_args


def run(command: list[str]) -> int:
    print(f"==> Running: {' '.join(command)}")
    return subprocess.call(command)


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(json.dumps(item) for item in items) + "]"


def detect_output_project(forwarded: list[str], default: str = "bills_tracer") -> str:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--output-project", default=default)
    args, _ = parser.parse_known_args(forwarded)
    return sanitize_segment(str(args.output_project))


def render_pipeline_config(
    *,
    pipeline_name: str,
    pipeline_description: str,
    output_root: str,
    steps: list[dict],
    default_timeout_seconds: int = 7200,
) -> str:
    lines = [
        "schema_version = 1",
        "",
        "[pipeline]",
        f"name = {json.dumps(pipeline_name)}",
        f"description = {json.dumps(pipeline_description)}",
        f"default_timeout_seconds = {default_timeout_seconds}",
        "",
        "[output]",
        f"root = {json.dumps(output_root)}",
        "",
        "[env]",
        'PYTHONUNBUFFERED = "1"',
        "",
    ]
    for step in steps:
        lines.extend(
            [
                "[[steps]]",
                f"id = {json.dumps(step['id'])}",
                f"name = {json.dumps(step.get('name', step['id']))}",
                f"command = {to_toml_list(step['command'])}",
                f"cwd = {json.dumps(step.get('cwd', '{repo_root}'))}",
                f"timeout_seconds = {int(step.get('timeout_seconds', default_timeout_seconds))}",
                f"retries = {int(step.get('retries', 0))}",
                f"depends_on = {to_toml_list([str(item) for item in step.get('depends_on', [])])}",
                f"artifacts = {to_toml_list([str(item) for item in step.get('artifacts', [])])}",
                "",
            ]
        )
    return "\n".join(lines)


def run_pipeline_steps(
    *,
    repo_root: Path,
    python_exe: str,
    pipeline_name: str,
    pipeline_description: str,
    output_root: str,
    steps: list[dict],
    run_id_prefix: str = "",
) -> int:
    config_text = render_pipeline_config(
        pipeline_name=pipeline_name,
        pipeline_description=pipeline_description,
        output_root=output_root,
        steps=steps,
    )
    temp_dir = repo_root / "temp"
    temp_dir.mkdir(parents=True, exist_ok=True)
    run_id = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    runner_entry = repo_root / "tools" / "verify" / "pipeline_runner.py"

    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".toml",
        prefix=f"verify_pipeline_{sanitize_segment(pipeline_name)}_",
        dir=temp_dir,
        delete=False,
        encoding="utf-8",
    ) as handle:
        config_path = Path(handle.name)
        handle.write(config_text)

    try:
        command = [python_exe, str(runner_entry), "--config", str(config_path)]
        if run_id_prefix:
            command.extend(["--run-id", f"{sanitize_segment(run_id_prefix)}_{run_id}"])
        return run(command)
    finally:
        config_path.unlink(missing_ok=True)


def run_bills_workflow(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    assert_no_legacy_flags(forwarded, source="verify bills")
    effective_forwarded = list(forwarded)
    if "--formats" not in effective_forwarded:
        effective_forwarded = [*effective_forwarded, "--formats", "md,json,tex,rst"]
    output_project = detect_output_project(forwarded)
    flow_entry = repo_root / "tools" / "flows" / "bills_tracer_flow.py"
    latest_root = resolve_artifact_latest_dir(repo_root, output_project)
    steps = [
        {
            "id": "bills_tracer",
            "name": "Run bills tracer flow",
            "command": [python_exe, str(flow_entry), *effective_forwarded],
            "cwd": "{repo_root}",
            "timeout_seconds": 7200,
            "depends_on": [],
            "artifacts": [
                str(latest_root / "test_summary.json"),
                str(latest_root / "test_python_output.log"),
            ],
        }
    ]
    return run_pipeline_steps(
        repo_root=repo_root,
        python_exe=python_exe,
        pipeline_name="verify_bills",
        pipeline_description="verify workflow bills",
        output_root=str(resolve_logic_pipeline_root(repo_root, "verify_bills")),
        steps=steps,
        run_id_prefix=output_project,
    )


def resolve_project_output_root(
    repo_root: Path,
    project: str,
    preferred_group: str = "artifact",
) -> Path:
    group = str(preferred_group).strip().lower()
    if group == "artifact":
        return resolve_artifact_project_root(repo_root, project)
    if group == "logic":
        return (repo_root / "build" / "tests" / "logic" / sanitize_segment(project)).resolve()
    if group == "runtime":
        return (repo_root / "build" / "tests" / "runtime" / sanitize_segment(project)).resolve()
    raise ValueError(f"Unsupported output group: {preferred_group}")


def resolve_project_latest_output_root(
    repo_root: Path,
    project: str,
    preferred_group: str = "artifact",
) -> Path:
    root = resolve_project_output_root(repo_root, project, preferred_group)
    if preferred_group == "artifact":
        return root / "latest"
    return root


def load_test_summary(repo_root: Path, project: str) -> dict | None:
    summary_path = (
        resolve_project_latest_output_root(repo_root, project) / "test_summary.json"
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
        resolve_project_latest_output_root(repo_root, project)
        / "test_python_output.log"
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
        default="md,json,tex,rst",
    )
    parser.add_argument(
        "--compare-scope",
        default="all",
        choices=["all", "md", "json", "tex", "rst"],
    )
    parser.add_argument(
        "--build-scope",
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
    assert_no_legacy_flags(passthrough, source="verify bills-parallel-smoke")
    if args.max_performance_regression < 0:
        print("[ERROR] --max-performance-regression must be >= 0.")
        return 2

    flow_entry = repo_root / "tools" / "flows" / "bills_tracer_flow.py"

    model_cmd = [
        python_exe,
        str(flow_entry),
        "--export-pipeline",
        "model-first",
        "--output-project",
        args.model_project,
        "--run-tag",
        "parallel_model",
        "--formats",
        args.formats,
        "--build-scope",
        args.build_scope,
        *passthrough,
    ]
    json_cmd = [
        python_exe,
        str(flow_entry),
        "--export-pipeline",
        "json-first",
        "--output-project",
        args.json_project,
        "--run-tag",
        "parallel_json",
        "--formats",
        args.formats,
        "--build-scope",
        args.build_scope,
        *passthrough,
    ]
    model_rev_project = f"{args.model_project}__rev"
    json_rev_project = f"{args.json_project}__rev"
    model_cmd_rev = [
        python_exe,
        str(flow_entry),
        "--export-pipeline",
        "model-first",
        "--output-project",
        model_rev_project,
        "--run-tag",
        "parallel_model_rev",
        "--formats",
        args.formats,
        "--build-scope",
        args.build_scope,
        *passthrough,
    ]
    json_cmd_rev = [
        python_exe,
        str(flow_entry),
        "--export-pipeline",
        "json-first",
        "--output-project",
        json_rev_project,
        "--run-tag",
        "parallel_json_rev",
        "--formats",
        args.formats,
        "--build-scope",
        args.build_scope,
        *passthrough,
    ]

    pipeline_steps: list[dict] = []
    compare_dependencies = ["model_primary", "json_primary"]
    if args.enforce_performance_gate:
        pipeline_steps.extend(
            [
                {
                    "id": "model_primary",
                    "name": "Perf-A model-first",
                    "command": model_cmd,
                    "depends_on": [],
                },
                {
                    "id": "json_primary",
                    "name": "Perf-A json-first",
                    "command": json_cmd,
                    "depends_on": ["model_primary"],
                },
                {
                    "id": "json_rev",
                    "name": "Perf-B json-first",
                    "command": json_cmd_rev,
                    "depends_on": ["json_primary"],
                },
                {
                    "id": "model_rev",
                    "name": "Perf-B model-first",
                    "command": model_cmd_rev,
                    "depends_on": ["json_rev"],
                },
            ]
        )
    else:
        pipeline_steps.extend(
            [
                {
                    "id": "model_primary",
                    "name": "Model-first run",
                    "command": model_cmd,
                    "depends_on": [],
                },
                {
                    "id": "json_primary",
                    "name": "Json-first run",
                    "command": json_cmd,
                    "depends_on": [],
                },
            ]
        )

    compare_entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
    pipeline_steps.append(
        {
            "id": "snapshot_compare",
            "name": "Compare snapshots",
            "command": [
                python_exe,
                str(compare_entry),
                "--output-group",
                "artifact",
                "--compare-projects",
                args.model_project,
                args.json_project,
                "--compare-scope",
                args.compare_scope,
            ],
            "depends_on": compare_dependencies,
            "timeout_seconds": 1800,
        }
    )
    pipeline_code = run_pipeline_steps(
        repo_root=repo_root,
        python_exe=python_exe,
        pipeline_name="verify_bills_parallel_smoke",
        pipeline_description="verify bills parallel smoke/gate via TOML pipeline",
        output_root=str(
            resolve_logic_pipeline_root(repo_root, "verify_bills_parallel_smoke")
        ),
        steps=pipeline_steps,
        run_id_prefix=f"{args.model_project}_{args.json_project}",
    )
    if pipeline_code != 0:
        return pipeline_code

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
        help="Build preset for bills_core dual-mode check.",
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
        bills_code = run_bills_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
        )
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
            "md,json,tex,rst",
            "--compare-scope",
            "all",
            "--build-scope",
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


def run_pipeline_workflow(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
    default_config: str = "",
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--config", default=default_config)
    parser.add_argument("--run-id", default="")
    parser.add_argument("--list-steps", action="store_true")
    args, passthrough = parser.parse_known_args(forwarded)

    config_path = args.config.strip()
    if not config_path:
        print("[ERROR] pipeline-run requires --config <path>.")
        return 2

    runner_entry = repo_root / "tools" / "verify" / "pipeline_runner.py"
    command = [python_exe, str(runner_entry), "--config", config_path]
    if args.run_id.strip():
        command.extend(["--run-id", args.run_id.strip()])
    if args.list_steps:
        command.append("--list-steps")
    command.extend(passthrough)
    return run(command)


def run_reporting_tools(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--skip-compile2pdf", action="store_true")
    parser.add_argument("--skip-graph", action="store_true")
    args, passthrough = parser.parse_known_args(forwarded)

    if passthrough:
        print(
            "[WARN] reporting-tools ignores extra args: "
            f"{' '.join(passthrough)}"
        )

    if not args.skip_compile2pdf:
        code = run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
            default_config="tools/verify/pipelines/reporting_compile2pdf.toml",
        )
        if code != 0:
            return code

    if not args.skip_graph:
        code = run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
            default_config="tools/verify/pipelines/reporting_graph_generator.toml",
        )
        if code != 0:
            return code

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
            "log-cli-test",
            "report-snapshot",
            "report-pipeline-compare",
            "pipeline-run",
            "pipeline-bills",
            "pipeline-log-generator",
            "reporting-compile2pdf",
            "reporting-graph",
            "reporting-tools",
        ],
        help=(
            "bills=build+CLI test, logic-tests=unit/component style checks, "
            "artifact-tests=integration/e2e/snapshot style checks, all-tests=logic+artifact, "
            "module-mode-check=dual-channel core build with BILLS_ENABLE_MODULES=OFF/ON, "
            "tools-layer-check=check tools/* layering dependency rules, "
            "import-layer-check=check call-layer include/import policy for C++ sources, "
            "boundary-layer-check=check boundary-layer include allowlist policy for C++ sources, "
            "bills-build=compile bills_cli with --preset/--scope, "
            "bills-parallel-smoke=run model-first/json-first bills in parallel and compare outputs, "
            "report-consistency-gate=run full model/json consistency gate with performance threshold, "
            "core-build=compile bills_core with --preset, core-abi=run ABI smoke tests, "
            "log-build=compile log_generator, "
            "log-cli-test=build log_generator and run CLI command tests, "
            "report-snapshot=compare exported reports with frozen snapshots, "
            "report-pipeline-compare=compare model-first and json-first outputs, "
            "pipeline-run=run TOML workflow via pipeline_runner, "
            "pipeline-bills=run bills pipeline config, "
            "pipeline-log-generator=run log_generator pipeline config, "
            "reporting-compile2pdf=run compile2pdf tooling workflow, "
            "reporting-graph=run graph_generator tooling workflow, "
            "reporting-tools=run compile2pdf+graph tooling workflows"
        ),
    )
    parser.add_argument(
        "extra",
        nargs=argparse.REMAINDER,
        help="Forwarded arguments for the selected workflow",
    )
    args = parser.parse_args()

    repo_root = REPO_ROOT
    python_exe = sys.executable
    forwarded = normalize_extra(args.extra)

    if args.workflow == "bills":
        return run_bills_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

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
            "md,json,tex,rst",
            "--compare-scope",
            "all",
            "--build-scope",
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
        entry = repo_root / "tools" / "flows" / "build_bills_master.py"
        if not forwarded:
            forwarded = ["--preset", "debug", "--scope", "shared"]
        assert_no_legacy_flags(forwarded, source="verify bills-build")
        return run([python_exe, str(entry), *forwarded])

    if args.workflow == "core-build":
        entry = repo_root / "tools" / "flows" / "build_bills_core.py"
        if not forwarded:
            forwarded = ["--preset", "debug", "--shared"]
        assert_no_legacy_flags(forwarded, source="verify core-build")
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

    if args.workflow == "pipeline-run":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "pipeline-bills":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
            default_config="tools/verify/pipelines/bills_tracer_single.toml",
        )

    if args.workflow == "pipeline-log-generator":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
            default_config="tools/verify/pipelines/log_generator_artifact.toml",
        )

    if args.workflow == "reporting-compile2pdf":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
            default_config="tools/verify/pipelines/reporting_compile2pdf.toml",
        )

    if args.workflow == "reporting-graph":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
            default_config="tools/verify/pipelines/reporting_graph_generator.toml",
        )

    if args.workflow == "reporting-tools":
        return run_reporting_tools(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
        )

    if args.workflow == "log-cli-test":
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=forwarded,
            default_config="tools/verify/pipelines/log_generator_cli.toml",
        )

    if args.workflow == "log-build":
        if not forwarded:
            return run_pipeline_workflow(
                repo_root=repo_root,
                python_exe=python_exe,
                forwarded=[],
                default_config="tools/verify/pipelines/log_generator_build.toml",
            )
        entry = repo_root / "tools" / "flows" / "log_generator_flow.py"
        return run([python_exe, str(entry), *forwarded])

    raise AssertionError(f"Unhandled workflow: {args.workflow}")


if __name__ == "__main__":
    raise SystemExit(main())
