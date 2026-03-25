from __future__ import annotations

import json
from pathlib import Path

from tools.toolchain.core.path_ops import replace_path
from tools.toolchain.services.build_layout import (
    assert_no_legacy_layout,
    resolve_logic_pipeline_root,
)


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def evaluate_artifacts(repo_root: Path, artifact_specs: list[str]) -> list[dict]:
    results: list[dict] = []
    for spec in artifact_specs:
        assert_no_legacy_layout(spec, source="pipeline.artifacts")
        artifact_path = Path(spec)
        if not artifact_path.is_absolute():
            artifact_path = (repo_root / artifact_path).resolve()
        exists = artifact_path.exists()
        size_bytes = artifact_path.stat().st_size if exists and artifact_path.is_file() else 0
        results.append(
            {
                "path": artifact_path.as_posix(),
                "exists": exists,
                "size_bytes": size_bytes,
            }
        )
    return results


def resolve_output_root(repo_root: Path, output_cfg: dict, pipeline_name: str) -> Path:
    root_raw = str(output_cfg.get("root", "")).strip()
    if root_raw:
        assert_no_legacy_layout(root_raw, source=f"pipeline[{pipeline_name}].output.root")
        root_path = Path(root_raw)
        if not root_path.is_absolute():
            root_path = (repo_root / root_path).resolve()
        return root_path
    return resolve_logic_pipeline_root(repo_root, pipeline_name)


def sync_latest(output_root: Path, run_dir: Path) -> None:
    replace_path(run_dir / "logs", output_root / "logs")
    replace_path(run_dir / "pipeline_summary.json", output_root / "pipeline_summary.json")
    replace_path(run_dir / "run_manifest.json", output_root / "run_manifest.json")
    (output_root / "latest_run.txt").write_text(run_dir.name, encoding="utf-8")
