from __future__ import annotations

import json
import os
import shutil
from datetime import datetime
from pathlib import Path

from tools.toolchain.core.path_ops import replace_path
from tools.toolchain.services.build_layout import sanitize_segment, short_hash

from .locks import directory_lock

TEST_SUMMARY_FILENAME = "test_summary.json"
PYTHON_TEST_LOG_FILENAME = "test_python_output.log"
RUN_MANIFEST_FILENAME = "run_manifest.json"
RUNS_DIR_NAME = "runs"
LATEST_DIR_NAME = "latest"
LATEST_SYNC_LOCK_DIR_NAME = ".latest_sync.lock"
RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".manifest", ".pdb"}


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
    elapsed_seconds: float,
    return_code: int,
    stdout: str,
    stderr: str,
) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        f"started_at={started_at.isoformat(timespec='microseconds')}",
        f"completed_at={completed_at.isoformat(timespec='microseconds')}",
        f"elapsed_seconds={elapsed_seconds:.6f}",
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


def make_runtime_base_dir(
    runtime_project_root: Path,
    output_project: str,
    export_pipeline: str,
    run_tag: str,
) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    project_tag = sanitize_segment(output_project)
    pipeline_tag = sanitize_segment(export_pipeline)
    custom_tag = sanitize_segment(run_tag) if run_tag else "auto"
    project_short = project_tag[:12]
    pipeline_short = pipeline_tag[:10]
    custom_short = custom_tag[:10]
    fingerprint = short_hash(
        f"{project_tag}|{pipeline_tag}|{custom_tag}|{os.getpid()}",
        length=8,
    )
    folder = f"{timestamp}_{project_short}_{pipeline_short}_{custom_short}_{fingerprint}"
    runtime_base = runtime_project_root / RUNS_DIR_NAME / folder
    runtime_base.mkdir(parents=True, exist_ok=True)
    return runtime_base.resolve()


def make_run_output_dir(project_output_root: Path, run_id: str) -> Path:
    run_output_dir = project_output_root / RUNS_DIR_NAME / run_id
    run_output_dir.mkdir(parents=True, exist_ok=True)
    return run_output_dir.resolve()


def write_json_file(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
def sync_runtime_workspace(
    build_bin_dir: Path,
    workspace_dir: Path,
    *,
    config_dir: Path | None = None,
    notices_dir: Path | None = None,
) -> None:
    if not build_bin_dir.exists():
        raise FileNotFoundError(f"Dist output directory not found: {build_bin_dir}")

    workspace_dir.mkdir(parents=True, exist_ok=True)
    for stale in workspace_dir.iterdir():
        if stale.is_file() and stale.suffix.lower() in RUNTIME_SIDECAR_EXTS:
            stale.unlink()

    copied_files: list[str] = []
    for entry in sorted(build_bin_dir.iterdir(), key=lambda path: path.name.lower()):
        if not entry.is_file():
            continue
        if entry.suffix.lower() not in RUNTIME_SIDECAR_EXTS:
            continue
        shutil.copy2(entry, workspace_dir / entry.name)
        copied_files.append(entry.name)

    replace_path(config_dir or (build_bin_dir / "config"), workspace_dir / "config")
    if notices_dir is not None:
        replace_path(notices_dir, workspace_dir / "notices")
    print(
        "==> Synced runtime artifacts to "
        f"{workspace_dir} ({len(copied_files)} files + config"
        f"{' + notices' if notices_dir is not None else ''})"
    )


def sync_latest_project_outputs(project_output_root: Path, run_output_dir: Path) -> None:
    lock_dir = project_output_root / LATEST_SYNC_LOCK_DIR_NAME
    with directory_lock(lock_dir):
        latest_output_root = project_output_root / LATEST_DIR_NAME
        if latest_output_root.exists():
            shutil.rmtree(latest_output_root, ignore_errors=True)
        latest_output_root.mkdir(parents=True, exist_ok=True)
        for legacy_name in [
            TEST_SUMMARY_FILENAME,
            PYTHON_TEST_LOG_FILENAME,
            RUN_MANIFEST_FILENAME,
            "logs",
            "cache",
            "exports",
            "record_templates",
            "txt2josn",
            "exported_files",
        ]:
            legacy_path = project_output_root / legacy_name
            if not legacy_path.exists():
                continue
            if legacy_path.is_dir():
                shutil.rmtree(legacy_path, ignore_errors=True)
            else:
                legacy_path.unlink(missing_ok=True)
        replace_path(
            run_output_dir / TEST_SUMMARY_FILENAME,
            latest_output_root / TEST_SUMMARY_FILENAME,
        )
        replace_path(
            run_output_dir / PYTHON_TEST_LOG_FILENAME,
            latest_output_root / PYTHON_TEST_LOG_FILENAME,
        )
        replace_path(
            run_output_dir / RUN_MANIFEST_FILENAME,
            latest_output_root / RUN_MANIFEST_FILENAME,
        )
        replace_path(run_output_dir / "logs", latest_output_root / "logs")
        replace_path(run_output_dir / "cache", latest_output_root / "cache")
        replace_path(run_output_dir / "exports", latest_output_root / "exports")
        replace_path(
            run_output_dir / "record_templates",
            latest_output_root / "record_templates",
        )
        (project_output_root / "latest_run.txt").write_text(
            run_output_dir.name,
            encoding="utf-8",
        )


def prune_old_runs(project_output_root: Path, max_runs: int) -> None:
    if max_runs <= 0:
        return
    runs_root = project_output_root / RUNS_DIR_NAME
    if not runs_root.exists():
        return
    run_dirs = [path for path in runs_root.iterdir() if path.is_dir()]
    if len(run_dirs) <= max_runs:
        return
    run_dirs.sort(key=lambda path: path.stat().st_mtime, reverse=True)
    for old_run_dir in run_dirs[max_runs:]:
        shutil.rmtree(old_run_dir, ignore_errors=True)
