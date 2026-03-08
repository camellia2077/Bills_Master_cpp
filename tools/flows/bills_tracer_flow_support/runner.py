from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
import time
from datetime import datetime
from pathlib import Path

from tools.toolchain.services.build_layout import (
    resolve_artifact_project_root,
    resolve_build_directory,
    resolve_runtime_project_root,
    resolve_runtime_workspace_dir,
    sanitize_segment,
)

from .artifacts import (
    PYTHON_TEST_LOG_FILENAME,
    RUN_MANIFEST_FILENAME,
    TEST_SUMMARY_FILENAME,
    load_test_summary,
    make_run_output_dir,
    make_runtime_base_dir,
    prune_old_runs,
    sync_latest_project_outputs,
    sync_runtime_workspace,
    write_json_file,
    write_python_test_log,
)
from .cmake_dist import build_cli, resolve_build_dir
from .config_writer import (
    cmake_format_defines,
    parse_formats,
    write_runtime_export_formats_config,
    write_temp_test_config,
)
from .models import BillsTracerRequest, BillsTracerRunPaths

DEFAULT_OUTPUT_PROJECT = "bills_tracer"
DEFAULT_BUILD_SCOPE = "shared"
DEFAULT_MAX_RUNS = 20


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Prepare bill_master_cli into dist/cmake and run command-line tests."
    )
    parser.add_argument(
        "--dist-scope",
        default=DEFAULT_BUILD_SCOPE,
        choices=["isolated", "shared"],
        help="isolated=dedicated dist dir per test run; shared=single reusable dist dir.",
    )
    parser.add_argument(
        "--preset",
        default="debug",
        choices=["debug", "release"],
        help="Preset to use for the CLI dist.",
    )
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--target", default="bill_master_cli")
    parser.add_argument("--bills-dir", default="testdata/bills")
    parser.add_argument("--formats", default="md")
    parser.add_argument("--ingest-mode", default="stepwise", choices=["stepwise", "ingest"])
    parser.add_argument("--ingest-write-json", action="store_true")
    parser.add_argument("--run-export-all", action="store_true")
    parser.add_argument(
        "--export-pipeline",
        default="model-first",
        choices=["legacy", "model-first", "json-first"],
    )
    parser.add_argument("--single-year", default="2025")
    parser.add_argument("--single-month", default="2025-01")
    parser.add_argument("--range-start", default="2025-03")
    parser.add_argument("--range-end", default="2025-04")
    parser.add_argument("--python", default=sys.executable)
    parser.add_argument("--output-project", default=DEFAULT_OUTPUT_PROJECT)
    parser.add_argument(
        "--run-tag",
        default="",
        help="Optional label for runtime workspace folder naming.",
    )
    parser.add_argument(
        "--max-runs",
        type=int,
        default=DEFAULT_MAX_RUNS,
        help="Maximum number of historical run artifacts to keep per output project. <=0 disables pruning.",
    )
    return parser


def build_request(args: argparse.Namespace, repo_root: Path) -> BillsTracerRequest:
    return BillsTracerRequest(
        dist_scope=str(args.dist_scope),
        preset=str(args.preset),
        generator=str(args.generator),
        target=str(args.target),
        bills_dir=(repo_root / str(args.bills_dir)).resolve(),
        export_formats=tuple(parse_formats(str(args.formats))),
        ingest_mode=str(args.ingest_mode),
        ingest_write_json=bool(args.ingest_write_json),
        run_export_all=bool(args.run_export_all),
        export_pipeline=str(args.export_pipeline),
        single_year=str(args.single_year),
        single_month=str(args.single_month),
        range_start=str(args.range_start),
        range_end=str(args.range_end),
        python_executable=str(args.python),
        output_project=sanitize_segment(str(args.output_project)),
        run_tag=str(args.run_tag),
        max_runs=int(args.max_runs),
    )


def build_run_paths(repo_root: Path, request: BillsTracerRequest) -> BillsTracerRunPaths:
    source_dir = repo_root / "apps" / "bills_cli"
    build_dir = resolve_build_dir(
        repo_root=repo_root,
        build_scope=request.dist_scope,
        build_preset=request.preset,
        output_project=request.output_project,
        export_pipeline=request.export_pipeline,
        formats=list(request.export_formats),
        generator=request.generator,
    )
    build_bin_dir = build_dir / "bin"
    test_root = repo_root / "tests"
    project_output_root = resolve_artifact_project_root(repo_root, request.output_project)
    runtime_project_root = resolve_runtime_project_root(repo_root, request.output_project)
    runtime_workspace_dir = resolve_runtime_workspace_dir(repo_root, request.output_project)
    test_runner = test_root / "suites" / "artifact" / "bills_master" / "run_tests.py"
    runtime_base_dir = make_runtime_base_dir(
        runtime_project_root=runtime_project_root,
        output_project=request.output_project,
        export_pipeline=request.export_pipeline,
        run_tag=request.run_tag,
    )
    run_id = runtime_base_dir.name
    run_output_dir = make_run_output_dir(project_output_root, run_id)
    return BillsTracerRunPaths(
        repo_root=repo_root,
        source_dir=source_dir,
        build_dir=build_dir,
        build_bin_dir=build_bin_dir,
        test_root=test_root,
        project_output_root=project_output_root,
        runtime_project_root=runtime_project_root,
        runtime_workspace_dir=runtime_workspace_dir,
        test_runner=test_runner,
        runtime_base_dir=runtime_base_dir,
        run_id=run_id,
        run_output_dir=run_output_dir,
        test_workdir=runtime_base_dir,
        import_dir=runtime_base_dir / "output" / "txt2josn",
        summary_path=run_output_dir / TEST_SUMMARY_FILENAME,
        python_test_log_path=run_output_dir / PYTHON_TEST_LOG_FILENAME,
    )


def _manifest_payload(
    paths: BillsTracerRunPaths,
    request: BillsTracerRequest,
    *,
    status: str,
    timestamp_key: str,
    timestamp_value: str,
    extra: dict | None = None,
) -> dict:
    payload = {
        "run_id": paths.run_id,
        "status": status,
        "output_project": request.output_project,
        "export_pipeline": request.export_pipeline,
        "formats": list(request.export_formats),
        "cmake_dist_dir": paths.build_dir.as_posix(),
        "runtime_workspace_dir": paths.runtime_workspace_dir.as_posix(),
        "runtime_base_dir": paths.runtime_base_dir.as_posix(),
        "run_output_dir": paths.run_output_dir.as_posix(),
        timestamp_key: timestamp_value,
    }
    if extra:
        payload.update(extra)
    return payload


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    repo_root = Path(__file__).resolve().parents[3]
    request = build_request(args, repo_root)
    paths = build_run_paths(repo_root, request)

    if not request.bills_dir.exists():
        print(f"Error: bills data directory not found: {request.bills_dir}")
        print("Hint: generate data first (e.g. via tests/generators/log_generator).")
        return 2
    if not paths.test_runner.exists():
        print(f"Error: test runner not found: {paths.test_runner}")
        return 2

    format_defines = cmake_format_defines(list(request.export_formats))
    build_spec = resolve_build_directory(
        repo_root,
        target="bills",
        preset=request.preset,
        scope=request.dist_scope,
        instance_id=paths.build_dir.name if request.dist_scope == "isolated" else "",
    )

    try:
        build_cli(
            source_dir=paths.source_dir,
            build_dir=paths.build_dir,
            generator=request.generator,
            build_type=build_spec.cmake_build_type,
            target=request.target,
            cmake_defines=format_defines,
        )
        sync_runtime_workspace(paths.build_bin_dir, paths.runtime_workspace_dir)
        write_runtime_export_formats_config(
            paths.runtime_workspace_dir,
            list(request.export_formats),
        )
    except subprocess.CalledProcessError as exc:
        print(f"Dist preparation failed with exit code {exc.returncode}.")
        return exc.returncode
    except FileNotFoundError as exc:
        print(f"Dist preparation failed: {exc}")
        return 2

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
        write_json_file(
            paths.run_output_dir / RUN_MANIFEST_FILENAME,
            _manifest_payload(
                paths,
                request,
                status="running",
                timestamp_key="created_at",
                timestamp_value=datetime.now().isoformat(timespec="seconds"),
            ),
        )
        write_temp_test_config(
            config_path=temp_config_path,
            workspace_dir=paths.runtime_workspace_dir,
            bills_dir=request.bills_dir,
            import_dir=paths.import_dir,
            runtime_base_dir=paths.runtime_base_dir,
            runtime_run_id=paths.run_id,
            runtime_output_dir=paths.run_output_dir,
            runtime_summary_path=paths.summary_path,
            run_export_all_tasks=request.run_export_all,
            export_formats=list(request.export_formats),
            ingest_mode=request.ingest_mode,
            ingest_write_json=request.ingest_write_json,
            export_pipeline=request.export_pipeline,
            output_project=request.output_project,
            single_year=request.single_year,
            single_month=request.single_month,
            range_start=request.range_start,
            range_end=request.range_end,
        )

        env = os.environ.copy()
        env["BILLS_MASTER_TEST_CONFIG"] = str(temp_config_path)
        print(f"==> {request.python_executable} {paths.test_runner}")
        test_cmd = [request.python_executable, str(paths.test_runner)]
        started_at = datetime.now()
        started_perf = time.perf_counter()
        test_result = subprocess.run(
            test_cmd,
            cwd=str(paths.test_workdir),
            env=env,
            capture_output=True,
            text=True,
            check=False,
        )
        elapsed_seconds = time.perf_counter() - started_perf
        completed_at = datetime.now()
        print(test_result.stdout, end="")
        if test_result.stderr:
            print(test_result.stderr, end="", file=sys.stderr)
        write_python_test_log(
            log_path=paths.python_test_log_path,
            command=test_cmd,
            started_at=started_at,
            completed_at=completed_at,
            elapsed_seconds=elapsed_seconds,
            return_code=test_result.returncode,
            stdout=test_result.stdout,
            stderr=test_result.stderr,
        )
        print(f"CLI python output log: {paths.python_test_log_path}")
        summary = load_test_summary(paths.summary_path)
        if summary is None:
            print(f"CLI tests did not produce summary JSON: {paths.summary_path}")
            write_json_file(
                paths.run_output_dir / RUN_MANIFEST_FILENAME,
                _manifest_payload(
                    paths,
                    request,
                    status="failed_no_summary",
                    timestamp_key="completed_at",
                    timestamp_value=datetime.now().isoformat(timespec="seconds"),
                    extra={"test_return_code": test_result.returncode},
                ),
            )
            return test_result.returncode if test_result.returncode != 0 else 3
        success = int(summary.get("success", 0))
        failed = int(summary.get("failed", 0))
        total = int(summary.get("total", 0))
        ok = bool(summary.get("ok", False))
        print(f"CLI test summary: ok={ok}, total={total}, success={success}, failed={failed}")
        write_json_file(
            paths.run_output_dir / RUN_MANIFEST_FILENAME,
            _manifest_payload(
                paths,
                request,
                status="ok" if ok and failed == 0 else "failed",
                timestamp_key="completed_at",
                timestamp_value=datetime.now().isoformat(timespec="seconds"),
                extra={
                    "test_return_code": test_result.returncode,
                    "summary": summary,
                },
            ),
        )
        sync_latest_project_outputs(paths.project_output_root, paths.run_output_dir)
        prune_old_runs(paths.project_output_root, request.max_runs)
        prune_old_runs(paths.runtime_project_root, request.max_runs)
        if not ok or failed > 0:
            return test_result.returncode if test_result.returncode != 0 else 1
    except subprocess.CalledProcessError as exc:
        print(f"CLI tests failed with exit code {exc.returncode}.")
        write_json_file(
            paths.run_output_dir / RUN_MANIFEST_FILENAME,
            _manifest_payload(
                paths,
                request,
                status="build_or_test_crashed",
                timestamp_key="completed_at",
                timestamp_value=datetime.now().isoformat(timespec="seconds"),
                extra={"return_code": exc.returncode},
            ),
        )
        return exc.returncode
    finally:
        temp_config_path.unlink(missing_ok=True)

    print("Dist preparation and CLI tests completed successfully.")
    return 0
