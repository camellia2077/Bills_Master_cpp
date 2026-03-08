from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class BillsTracerRequest:
    dist_scope: str
    preset: str
    generator: str
    target: str
    bills_dir: Path
    export_formats: tuple[str, ...]
    ingest_mode: str
    ingest_write_json: bool
    run_export_all: bool
    export_pipeline: str
    single_year: str
    single_month: str
    range_start: str
    range_end: str
    python_executable: str
    output_project: str
    run_tag: str
    max_runs: int


@dataclass(frozen=True)
class BillsTracerRunPaths:
    repo_root: Path
    source_dir: Path
    build_dir: Path
    build_bin_dir: Path
    test_root: Path
    project_output_root: Path
    runtime_project_root: Path
    runtime_workspace_dir: Path
    test_runner: Path
    runtime_base_dir: Path
    run_id: str
    run_output_dir: Path
    test_workdir: Path
    import_dir: Path
    summary_path: Path
    python_test_log_path: Path
