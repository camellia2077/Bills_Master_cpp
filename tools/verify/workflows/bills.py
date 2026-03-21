from __future__ import annotations

import argparse
from pathlib import Path

from tools.toolchain.services.build_layout import (
    assert_no_legacy_flags,
    resolve_artifact_latest_dir,
    resolve_logic_pipeline_root,
)
from tools.verify.report_snapshot_support import COMPARE_SCOPE_STANDARD_REPORT

from .common import (
    detect_output_project,
    validate_performance_regression,
    validate_test_summary,
)
from .pipeline_helpers import run_pipeline_steps

SUPPORTED_VERIFY_FORMATS = {"md", "json", "tex", "rst", "typ"}


def parse_requested_formats(forwarded: list[str]) -> list[str]:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--formats", default="md,json,tex,rst,typ")
    args, _ = parser.parse_known_args(forwarded)
    raw_formats = str(args.formats).strip()
    formats = [item.strip().lower() for item in raw_formats.split(",") if item.strip()]
    if not formats:
        raise ValueError("No valid format found in --formats.")

    deduped: list[str] = []
    for fmt in formats:
        if fmt not in SUPPORTED_VERIFY_FORMATS:
            raise ValueError(
                f"Unsupported format '{fmt}' for verify bills. "
                f"Supported: {', '.join(sorted(SUPPORTED_VERIFY_FORMATS))}."
            )
        if fmt not in deduped:
            deduped.append(fmt)
    return deduped


def build_snapshot_compare_steps(
    *,
    repo_root: Path,
    python_exe: str,
    output_project: str,
    formats: list[str],
    depends_on: list[str],
) -> list[dict]:
    compare_entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
    steps: list[dict] = []
    for fmt in formats:
        steps.append(
            {
                "id": f"snapshot_compare_{fmt}",
                "name": f"Compare {fmt} snapshots",
                "command": [
                    python_exe,
                    str(compare_entry),
                    "--project",
                    output_project,
                    "--compare-scope",
                    fmt,
                ],
                "cwd": "{repo_root}",
                "timeout_seconds": 1800,
                "depends_on": list(depends_on),
            }
        )
    if "json" in formats:
        steps.append(
            {
                "id": "snapshot_compare_standard_report",
                "name": "Compare standard-report goldens",
                "command": [
                    python_exe,
                    str(compare_entry),
                    "--project",
                    output_project,
                    "--compare-scope",
                    COMPARE_SCOPE_STANDARD_REPORT,
                ],
                "cwd": "{repo_root}",
                "timeout_seconds": 1800,
                "depends_on": list(depends_on),
            }
        )
    return steps


def run_bills_workflow(
    repo_root: Path,
    python_exe: str,
    forwarded: list[str],
) -> int:
    assert_no_legacy_flags(forwarded, source="verify bills")
    effective_forwarded = list(forwarded)
    if "--formats" not in effective_forwarded:
        effective_forwarded = [*effective_forwarded, "--formats", "md,json,tex,rst,typ"]
    output_project = detect_output_project(forwarded)
    try:
        requested_formats = parse_requested_formats(effective_forwarded)
    except ValueError as exc:
        print(f"[ERROR] {exc}")
        return 2
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
    steps.extend(
        build_snapshot_compare_steps(
            repo_root=repo_root,
            python_exe=python_exe,
            output_project=output_project,
            formats=requested_formats,
            depends_on=["bills_tracer"],
        )
    )
    return run_pipeline_steps(
        repo_root=repo_root,
        python_exe=python_exe,
        pipeline_name="verify_bills",
        pipeline_description="verify workflow bills",
        output_root=str(resolve_logic_pipeline_root(repo_root, "verify_bills")),
        steps=steps,
        run_id_prefix=output_project,
    )


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
        default="md,json,tex,rst,typ",
    )
    parser.add_argument(
        "--compare-scope",
        default="all",
        choices=["all", "md", "json", "tex", "rst", "typ", "standard-report"],
    )
    parser.add_argument(
        "--dist-scope",
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
        "--dist-scope",
        args.dist_scope,
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
        "--dist-scope",
        args.dist_scope,
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
        "--dist-scope",
        args.dist_scope,
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
        "--dist-scope",
        args.dist_scope,
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
        output_root=str(resolve_logic_pipeline_root(repo_root, "verify_bills_parallel_smoke")),
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
            "md,json,tex,rst,typ",
            "--compare-scope",
            "all",
            "--dist-scope",
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
