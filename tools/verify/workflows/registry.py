from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

from tools.toolchain.services.build_layout import assert_no_legacy_flags

from .bills import run_artifact_tests, run_bills_parallel_smoke, run_bills_workflow
from .common import run
from .logic import (
    run_boundary_layer_check,
    run_import_layer_check,
    run_logic_tests,
    run_module_mode_check,
    run_tools_layer_check,
)
from .pipeline_helpers import run_pipeline_workflow
from .reporting import run_reporting_tools

WorkflowHandler = Callable[[Path, str, list[str]], int]


@dataclass(frozen=True)
class WorkflowSpec:
    name: str
    help_text: str
    handler: WorkflowHandler


def _run_all_tests(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
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


def _run_report_consistency_gate(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
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


def _run_bills_dist(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    entry = repo_root / "tools" / "flows" / "build_bills_master.py"
    effective_forwarded = list(forwarded)
    if not effective_forwarded:
        effective_forwarded = ["--preset", "debug", "--scope", "shared"]
    assert_no_legacy_flags(effective_forwarded, source="verify bills-dist")
    return run([python_exe, str(entry), *effective_forwarded])


def _run_core_dist(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    entry = repo_root / "tools" / "flows" / "build_bills_core.py"
    effective_forwarded = list(forwarded)
    if not effective_forwarded:
        effective_forwarded = ["--preset", "debug", "--shared"]
    assert_no_legacy_flags(effective_forwarded, source="verify core-dist")
    return run([python_exe, str(entry), *effective_forwarded])


def _run_core_abi(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    entry = repo_root / "tests" / "suites" / "logic" / "bills_core_abi" / "run_tests.py"
    return run([python_exe, str(entry), *forwarded])


def _run_report_snapshot(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
    return run([python_exe, str(entry), *forwarded])


def _run_report_pipeline_compare(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    entry = repo_root / "tools" / "verify" / "check_report_snapshots.py"
    effective_forwarded = list(forwarded)
    if not effective_forwarded:
        effective_forwarded = [
            "--compare-projects",
            "bills_tracer_model_first",
            "bills_tracer_json_first",
            "--compare-scope",
            "md",
        ]
    return run([python_exe, str(entry), *effective_forwarded])


def _run_pipeline_bills(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    return run_pipeline_workflow(
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
        default_config="tools/verify/pipelines/bills_tracer_single.toml",
    )


def _run_pipeline_log_generator(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    return run_pipeline_workflow(
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
        default_config="tools/verify/pipelines/log_generator_artifact.toml",
    )


def _run_reporting_compile2pdf(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    return run_pipeline_workflow(
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
        default_config="tools/verify/pipelines/reporting_compile2pdf.toml",
    )


def _run_reporting_graph(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    return run_pipeline_workflow(
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
        default_config="tools/verify/pipelines/reporting_graph_generator.toml",
    )


def _run_log_cli_test(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    return run_pipeline_workflow(
        repo_root=repo_root,
        python_exe=python_exe,
        forwarded=forwarded,
        default_config="tools/verify/pipelines/log_generator_cli.toml",
    )


def _run_log_dist(repo_root: Path, python_exe: str, forwarded: list[str]) -> int:
    if not forwarded:
        return run_pipeline_workflow(
            repo_root=repo_root,
            python_exe=python_exe,
            forwarded=[],
            default_config="tools/verify/pipelines/log_generator_dist.toml",
        )
    entry = repo_root / "tools" / "flows" / "log_generator_flow.py"
    return run([python_exe, str(entry), *forwarded])


def workflow_specs() -> list[WorkflowSpec]:
    return [
        WorkflowSpec("bills", "dist+CLI test", run_bills_workflow),
        WorkflowSpec("logic-tests", "unit/component style checks", run_logic_tests),
        WorkflowSpec(
            "artifact-tests",
            "integration/e2e/snapshot style checks",
            run_artifact_tests,
        ),
        WorkflowSpec("all-tests", "logic+artifact", _run_all_tests),
        WorkflowSpec(
            "module-mode-check",
            "dual-channel core dist with BILLS_ENABLE_MODULES=OFF/ON",
            run_module_mode_check,
        ),
        WorkflowSpec(
            "tools-layer-check",
            "check tools/* layering dependency rules",
            run_tools_layer_check,
        ),
        WorkflowSpec(
            "import-layer-check",
            "check call-layer include/import policy for C++ sources",
            run_import_layer_check,
        ),
        WorkflowSpec(
            "boundary-layer-check",
            "check boundary-layer include allowlist policy for C++ sources",
            run_boundary_layer_check,
        ),
        WorkflowSpec(
            "bills-parallel-smoke",
            "run model-first/json-first bills in parallel and compare outputs",
            run_bills_parallel_smoke,
        ),
        WorkflowSpec(
            "report-consistency-gate",
            "run full model/json consistency gate with performance threshold",
            _run_report_consistency_gate,
        ),
        WorkflowSpec(
            "bills-dist",
            "prepare bills_cli into dist/cmake with --preset/--scope",
            _run_bills_dist,
        ),
        WorkflowSpec(
            "core-dist",
            "prepare bills_core into dist/cmake with --preset",
            _run_core_dist,
        ),
        WorkflowSpec("core-abi", "run ABI smoke tests", _run_core_abi),
        WorkflowSpec("log-dist", "prepare log_generator into dist/cmake", _run_log_dist),
        WorkflowSpec(
            "log-cli-test",
            "prepare log_generator and run CLI command tests",
            _run_log_cli_test,
        ),
        WorkflowSpec(
            "report-snapshot",
            "compare exported reports and standard-report goldens with frozen snapshots",
            _run_report_snapshot,
        ),
        WorkflowSpec(
            "report-pipeline-compare",
            "compare model-first and json-first outputs",
            _run_report_pipeline_compare,
        ),
        WorkflowSpec(
            "pipeline-run",
            "run TOML workflow via pipeline_runner",
            run_pipeline_workflow,
        ),
        WorkflowSpec(
            "pipeline-bills",
            "run bills pipeline config",
            _run_pipeline_bills,
        ),
        WorkflowSpec(
            "pipeline-log-generator",
            "run log_generator pipeline config",
            _run_pipeline_log_generator,
        ),
        WorkflowSpec(
            "reporting-compile2pdf",
            "run compile2pdf tooling workflow",
            _run_reporting_compile2pdf,
        ),
        WorkflowSpec(
            "reporting-graph",
            "run graph_generator tooling workflow",
            _run_reporting_graph,
        ),
        WorkflowSpec(
            "reporting-tools",
            "run compile2pdf+graph tooling workflows",
            run_reporting_tools,
        ),
    ]


def workflow_registry() -> dict[str, WorkflowSpec]:
    return {spec.name: spec for spec in workflow_specs()}


def workflow_help_text() -> str:
    return ", ".join(f"{spec.name}={spec.help_text}" for spec in workflow_specs())
