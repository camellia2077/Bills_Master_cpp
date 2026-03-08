from __future__ import annotations

import os
import sys
from datetime import datetime
from pathlib import Path

from .io_ops import replace_path, write_json_file
from .utils import run_command


REPO_ROOT = Path(__file__).resolve().parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    resolve_artifact_project_root,
    resolve_artifact_run_dir,
    resolve_runtime_project_root,
    resolve_runtime_run_dir,
)


def _resolve_generator_executable(build_bin_dir: Path) -> Path:
    exe_name = "generator.exe" if os.name == "nt" else "generator"
    return build_bin_dir / exe_name


def _resolve_generator_config(build_bin_dir: Path, project_dir: Path) -> Path | None:
    candidates = [
        build_bin_dir / "config" / "config.toml",
        project_dir / "src" / "config" / "config.toml",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def run_generate_to_artifact(
    *,
    repo_root: Path,
    project_dir: Path,
    build_dir: Path,
    output_project: str,
    start_year: int,
    end_year: int,
) -> int:
    if start_year > end_year:
        print("[ERROR] start_year 不能大于 end_year。")
        return 2

    build_bin_dir = build_dir / "bin"
    generator_exe = _resolve_generator_executable(build_bin_dir)
    if not generator_exe.exists():
        print(f"[ERROR] 生成器可执行文件不存在: {generator_exe}")
        print("Hint: 先执行 build 命令编译 log_generator。")
        return 2

    config_src = _resolve_generator_config(build_bin_dir, project_dir)
    if config_src is None:
        print("[ERROR] 未找到 log_generator 配置文件 config.toml。")
        return 2

    run_id = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    runtime_root = resolve_runtime_project_root(repo_root, output_project)
    runtime_run_dir = resolve_runtime_run_dir(repo_root, output_project, run_id)
    runtime_run_dir.mkdir(parents=True, exist_ok=True)
    replace_path(config_src, runtime_run_dir / "config.toml")

    run_command(
        [str(generator_exe), "--double", str(start_year), str(end_year)],
        cwd=runtime_run_dir,
    )

    generated_dir = runtime_run_dir / "bills_output_from_config"
    if not generated_dir.exists():
        print(f"[ERROR] 生成目录不存在: {generated_dir}")
        return 2

    artifact_root = resolve_artifact_project_root(repo_root, output_project)
    artifact_run_dir = resolve_artifact_run_dir(repo_root, output_project, run_id)
    run_dataset_dir = artifact_run_dir / "bills_output_from_config"
    latest_dataset_dir = artifact_root / "latest" / "bills_output_from_config"
    replace_path(generated_dir, run_dataset_dir)
    replace_path(run_dataset_dir, latest_dataset_dir)
    (artifact_root / "latest_run.txt").write_text(run_id, encoding="utf-8")
    write_json_file(
        artifact_run_dir / "run_manifest.json",
        {
            "run_id": run_id,
            "status": "ok",
            "output_project": output_project,
            "start_year": start_year,
            "end_year": end_year,
            "generator_executable": generator_exe.as_posix(),
            "runtime_run_dir": runtime_run_dir.as_posix(),
            "artifact_run_dir": artifact_run_dir.as_posix(),
            "created_at": datetime.now().isoformat(timespec="seconds"),
        },
    )
    print(f"==> Generated dataset run_id={run_id}")
    print(f"==> Latest artifact dataset: {latest_dataset_dir}")
    return 0


def promote_artifact_to_fixtures(
    *,
    repo_root: Path,
    output_project: str,
    run_id: str,
) -> int:
    artifact_root = resolve_artifact_project_root(repo_root, output_project)
    if run_id:
        source_dir = artifact_root / "runs" / run_id / "bills_output_from_config"
    else:
        source_dir = artifact_root / "latest" / "bills_output_from_config"

    if not source_dir.exists():
        print(f"[ERROR] 找不到可 promote 的数据目录: {source_dir}")
        print("Hint: 先执行 generate 命令，或指定正确的 --run-id。")
        return 2

    fixtures_dir = repo_root / "tests" / "fixtures" / "bills"
    replace_path(source_dir, fixtures_dir)
    write_json_file(
        artifact_root / "last_promote.json",
        {
            "source_dir": source_dir.as_posix(),
            "target_dir": fixtures_dir.as_posix(),
            "promoted_at": datetime.now().isoformat(timespec="seconds"),
        },
    )
    print(f"==> Promoted dataset to fixtures: {fixtures_dir}")
    return 0
