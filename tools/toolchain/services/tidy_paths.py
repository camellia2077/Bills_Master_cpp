from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..core.context import Context


@dataclass(frozen=True)
class TidyPaths:
    root: Path
    run_root: Path
    raw_root: Path
    latest_build_log: Path
    latest_summary: Path
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


def resolve_tidy_paths(ctx: Context, build_dir_name: str | None = None) -> TidyPaths:
    normalized_build_dir_name = (build_dir_name or "").strip() or "build_tidy"
    root = ctx.temp_root / "tidy"
    run_root = root / "runs"
    raw_root = root / "raw"
    build_dir = ctx.repo_root / "apps" / "bills_cli" / normalized_build_dir_name
    paths = TidyPaths(
        root=root,
        run_root=run_root,
        raw_root=raw_root,
        latest_build_log=raw_root / "build.log",
        latest_summary=raw_root / "summary.json",
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
        paths.tasks_dir,
        paths.tasks_done_dir,
        paths.refresh_dir,
    ]:
        directory.mkdir(parents=True, exist_ok=True)
    return paths
