from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..core.context import Context
from .build_layout import resolve_build_directory


@dataclass(frozen=True)
class TidyPaths:
    root: Path
    run_root: Path
    raw_root: Path
    state_root: Path
    batch_state_dir: Path
    latest_build_log: Path
    latest_diagnostics_path: Path
    latest_summary: Path
    latest_state_path: Path
    tasks_dir: Path
    tasks_done_dir: Path
    tasks_manifest: Path
    tasks_summary: Path
    batch_state_path: Path
    batch_status_path: Path
    refresh_state_path: Path
    verify_result_path: Path
    tidy_result_path: Path
    checkpoint_path: Path
    refresh_dir: Path
    build_dir: Path
    build_log_path: Path
    compile_commands_path: Path


def resolve_tidy_paths(ctx: Context) -> TidyPaths:
    root = ctx.temp_root / "tidy"
    run_root = root / "runs"
    raw_root = root / "raw"
    state_root = root / "state"
    batch_state_dir = state_root / "batches"
    build_dir = resolve_build_directory(
        ctx.repo_root,
        target="bills-tracer-cli",
        preset="tidy",
        scope="shared",
    ).build_dir
    paths = TidyPaths(
        root=root,
        run_root=run_root,
        raw_root=raw_root,
        state_root=state_root,
        batch_state_dir=batch_state_dir,
        latest_build_log=raw_root / "build.log",
        latest_diagnostics_path=raw_root / "diagnostics.jsonl",
        latest_summary=raw_root / "summary.json",
        latest_state_path=state_root / "latest.json",
        tasks_dir=root / "tasks",
        tasks_done_dir=root / "tasks_done",
        tasks_manifest=root / "tasks" / "manifest.json",
        tasks_summary=root / "tasks" / "tasks_summary.md",
        batch_state_path=root / "batch_state.json",
        batch_status_path=root / "batch_status.json",
        refresh_state_path=root / "refresh_state.json",
        verify_result_path=root / "verify_result.json",
        tidy_result_path=root / "tidy_result.json",
        checkpoint_path=root / "tidy_batch_checkpoint.json",
        refresh_dir=root / "refresh",
        build_dir=build_dir,
        build_log_path=build_dir / "build.log",
        compile_commands_path=build_dir / "compile_commands.json",
    )
    for directory in [
        paths.root,
        paths.run_root,
        paths.raw_root,
        paths.state_root,
        paths.batch_state_dir,
        paths.tasks_dir,
        paths.tasks_done_dir,
        paths.refresh_dir,
    ]:
        directory.mkdir(parents=True, exist_ok=True)
    return paths
