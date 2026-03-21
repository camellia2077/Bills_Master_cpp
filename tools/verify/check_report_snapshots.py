#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    resolve_artifact_latest_dir,
    resolve_artifact_project_root,
)
from tools.verify.report_snapshot_support import (
    VALID_COMPARE_SCOPES,
    compare_mode_for_manifest_item,
    normalize_content,
    should_collect_extra_source,
    should_compare_manifest_item,
)


def compare_file(current_path: Path, baseline_path: Path, *, compare_mode: str) -> bool:
    current_text = normalize_content(
        current_path.read_text(encoding="utf-8"),
        compare_mode,
    )
    baseline_text = normalize_content(
        baseline_path.read_text(encoding="utf-8"),
        compare_mode,
    )
    return current_text == baseline_text


def compare_against_baseline(
    manifest: dict,
    current_root: Path,
    baseline_root: Path,
    compare_scope: str,
) -> int:
    failed: list[str] = []
    compared = 0
    for key, item in manifest.items():
        if not should_compare_manifest_item(item, compare_scope):
            continue
        compared += 1

        source_rel = item["source"]
        baseline_rel = item["baseline"]
        current_path = current_root / source_rel
        baseline_path = baseline_root / baseline_rel

        if not current_path.exists():
            failed.append(f"{key}: missing current file {current_path}")
            continue
        if not baseline_path.exists():
            failed.append(f"{key}: missing baseline file {baseline_path}")
            continue
        if not compare_file(
            current_path,
            baseline_path,
            compare_mode=compare_mode_for_manifest_item(item),
        ):
            failed.append(f"{key}: content mismatch")

    if failed:
        print("[FAILED] Snapshot comparison failed:")
        for item in failed:
            print(f"  - {item}")
        return 1

    print(f"[OK] Snapshot comparison passed. compared={compared}, scope={compare_scope}")
    return 0


def compare_between_projects(
    manifest: dict,
    project_a_root: Path,
    project_b_root: Path,
    compare_scope: str,
) -> int:
    manifest_sources = {
        item["source"]
        for item in manifest.values()
        if should_compare_manifest_item(item, compare_scope)
    }
    files_a = collect_scoped_files(project_a_root, compare_scope)
    files_b = collect_scoped_files(project_b_root, compare_scope)
    union_rel_paths = set(files_a.keys()) | set(files_b.keys())

    failed: list[str] = []
    compared = 0
    for key, item in manifest.items():
        if not should_compare_manifest_item(item, compare_scope):
            continue
        compared += 1

        source_rel = item["source"]
        project_a_path = project_a_root / source_rel
        project_b_path = project_b_root / source_rel

        if not project_a_path.exists():
            failed.append(f"{key}: missing project A file {project_a_path}")
            continue
        if not project_b_path.exists():
            failed.append(f"{key}: missing project B file {project_b_path}")
            continue
        if not compare_file(
            project_a_path,
            project_b_path,
            compare_mode=compare_mode_for_manifest_item(item),
        ):
            failed.append(f"{key}: content mismatch")

    # Compare additional files that are not listed in manifest (e.g. tex).
    extra_rel_paths = sorted(union_rel_paths - manifest_sources)
    for relative in extra_rel_paths:
        project_a_path = files_a.get(relative)
        project_b_path = files_b.get(relative)
        if project_a_path is None:
            failed.append(f"missing project A file {project_a_root / relative}")
            continue
        if project_b_path is None:
            failed.append(f"missing project B file {project_b_root / relative}")
            continue
        compared += 1
        if not compare_file(project_a_path, project_b_path, compare_mode="text"):
            failed.append(f"{relative}: content mismatch")

    if failed:
        print("[FAILED] Project-to-project snapshot comparison failed:")
        for item in failed:
            print(f"  - {item}")
        return 1

    print(
        "[OK] Project-to-project snapshot comparison passed. "
        f"compared={compared}, scope={compare_scope}, extras={len(extra_rel_paths)}"
    )
    return 0


def collect_scoped_files(root: Path, compare_scope: str) -> dict[str, Path]:
    files: dict[str, Path] = {}
    if not root.exists():
        return files
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if not should_collect_extra_source(relative, compare_scope):
            continue
        files[relative] = path
    return files


def resolve_project_output_root(repo_root: Path, project: str, output_group: str) -> Path:
    if output_group != "artifact":
        raise ValueError(
            f"Unsupported output_group '{output_group}'. Only 'artifact' is supported."
        )
    root = resolve_artifact_project_root(repo_root, project)
    if output_group == "artifact":
        return resolve_artifact_latest_dir(repo_root, project)
    return root


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare exported report outputs against frozen snapshots.",
    )
    parser.add_argument(
        "--project",
        default="bills_tracer",
        help="Project output directory name (default: bills_tracer).",
    )
    parser.add_argument(
        "--output-group",
        default="artifact",
        help=("Output group under dist/tests (default: artifact)."),
    )
    parser.add_argument(
        "--compare-projects",
        nargs=2,
        metavar=("PROJECT_A", "PROJECT_B"),
        help=(
            "Compare outputs between two projects under selected output group. "
            "Example: --compare-projects bills_tracer_model_first bills_tracer_json_first"
        ),
    )
    parser.add_argument(
        "--compare-scope",
        default="all",
        choices=VALID_COMPARE_SCOPES,
        help="Limit comparison to selected file type.",
    )
    args = parser.parse_args()

    repo_root = REPO_ROOT
    baseline_root = repo_root / "tests" / "baseline" / "report_snapshots"
    manifest_path = baseline_root / "manifest.json"

    if not manifest_path.exists():
        print(f"[ERROR] Snapshot manifest missing: {manifest_path}")
        return 2

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    if args.compare_projects:
        project_a, project_b = args.compare_projects
        project_a_root = (
            resolve_project_output_root(repo_root, project_a, args.output_group) / "exports"
        )
        project_b_root = (
            resolve_project_output_root(repo_root, project_b, args.output_group) / "exports"
        )
        return compare_between_projects(
            manifest=manifest,
            project_a_root=project_a_root,
            project_b_root=project_b_root,
            compare_scope=args.compare_scope,
        )

    current_root = (
        resolve_project_output_root(repo_root, args.project, args.output_group) / "exports"
    )
    return compare_against_baseline(
        manifest=manifest,
        current_root=current_root,
        baseline_root=baseline_root,
        compare_scope=args.compare_scope,
    )


if __name__ == "__main__":
    raise SystemExit(main())
