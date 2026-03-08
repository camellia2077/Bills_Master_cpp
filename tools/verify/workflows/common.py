from __future__ import annotations

import argparse
import json
import subprocess
from datetime import datetime
from pathlib import Path
from statistics import mean, median

from tools.toolchain.services.build_layout import (
    resolve_artifact_project_root,
    resolve_tests_root,
    sanitize_segment,
)


def normalize_extra(extra_args: list[str]) -> list[str]:
    if extra_args and extra_args[0] == "--":
        return extra_args[1:]
    return extra_args


def run(command: list[str]) -> int:
    print(f"==> Running: {' '.join(command)}")
    return subprocess.call(command)


def detect_output_project(forwarded: list[str], default: str = "bills_tracer") -> str:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--output-project", default=default)
    args, _ = parser.parse_known_args(forwarded)
    return sanitize_segment(str(args.output_project))


def resolve_project_output_root(
    repo_root: Path,
    project: str,
    preferred_group: str = "artifact",
) -> Path:
    group = str(preferred_group).strip().lower()
    if group == "artifact":
        return resolve_artifact_project_root(repo_root, project)
    if group == "logic":
        return (resolve_tests_root(repo_root) / "logic" / sanitize_segment(project)).resolve()
    if group == "runtime":
        return (resolve_tests_root(repo_root) / "runtime" / sanitize_segment(project)).resolve()
    raise ValueError(f"Unsupported output group: {preferred_group}")


def resolve_project_latest_output_root(
    repo_root: Path,
    project: str,
    preferred_group: str = "artifact",
) -> Path:
    root = resolve_project_output_root(repo_root, project, preferred_group)
    if preferred_group == "artifact":
        return root / "latest"
    return root


def load_test_summary(repo_root: Path, project: str) -> dict | None:
    summary_path = resolve_project_latest_output_root(repo_root, project) / "test_summary.json"
    if not summary_path.exists():
        print(f"[ERROR] Missing test summary: {summary_path}")
        return None
    try:
        return json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"[ERROR] Invalid test summary JSON: {summary_path} ({exc})")
        return None


def validate_test_summary(repo_root: Path, project: str) -> int:
    summary = load_test_summary(repo_root, project)
    if summary is None:
        return 2
    ok = bool(summary.get("ok", False))
    failed = int(summary.get("failed", 0))
    if not ok or failed > 0:
        print(
            f"[FAILED] Test summary validation failed: project={project}, ok={ok}, failed={failed}"
        )
        return 1
    print(
        "[OK] Test summary validation passed: "
        f"project={project}, total={summary.get('total', 0)}, "
        f"success={summary.get('success', 0)}, failed={failed}"
    )
    return 0


def load_python_test_duration_seconds(repo_root: Path, project: str) -> float | None:
    log_path = resolve_project_latest_output_root(repo_root, project) / "test_python_output.log"
    if not log_path.exists():
        print(f"[ERROR] Missing python test log: {log_path}")
        return None

    started_at_raw = ""
    completed_at_raw = ""
    elapsed_raw = ""
    try:
        for line in log_path.read_text(encoding="utf-8").splitlines():
            if line.startswith("started_at="):
                started_at_raw = line.split("=", 1)[1].strip()
            elif line.startswith("completed_at="):
                completed_at_raw = line.split("=", 1)[1].strip()
            elif line.startswith("elapsed_seconds="):
                elapsed_raw = line.split("=", 1)[1].strip()
            if elapsed_raw or (started_at_raw and completed_at_raw):
                break
    except OSError as exc:
        print(f"[ERROR] Unable to read python test log: {log_path} ({exc})")
        return None

    if elapsed_raw:
        try:
            elapsed = float(elapsed_raw)
        except ValueError as exc:
            print(f"[ERROR] Invalid elapsed_seconds in python test log: {log_path} ({exc})")
            return None
        if elapsed < 0:
            print(f"[ERROR] Negative elapsed_seconds in python test log: {log_path}")
            return None
        return elapsed

    if not started_at_raw or not completed_at_raw:
        print(f"[ERROR] Missing started_at/completed_at in python test log: {log_path}")
        return None

    try:
        started_at = datetime.fromisoformat(started_at_raw)
        completed_at = datetime.fromisoformat(completed_at_raw)
    except ValueError as exc:
        print(f"[ERROR] Invalid timestamp in python test log: {log_path} ({exc})")
        return None

    duration = (completed_at - started_at).total_seconds()
    if duration < 0:
        print(f"[ERROR] Negative duration from python test log: {log_path}")
        return None
    return duration


def validate_performance_regression(
    repo_root: Path,
    model_projects: list[str],
    json_projects: list[str],
    max_regression_ratio: float,
    stat_method: str,
) -> int:
    model_durations: list[float] = []
    json_durations: list[float] = []

    for project in model_projects:
        duration = load_python_test_duration_seconds(repo_root, project)
        if duration is None:
            return 2
        model_durations.append(duration)

    for project in json_projects:
        duration = load_python_test_duration_seconds(repo_root, project)
        if duration is None:
            return 2
        json_durations.append(duration)

    if not model_durations or not json_durations:
        print("[ERROR] Empty duration samples for performance gate.")
        return 2

    if any(duration <= 0 for duration in json_durations):
        print(
            "[WARN] Skip performance gate because json-first duration "
            f"is not positive: {json_durations}"
        )
        return 0

    if stat_method == "mean":
        model_duration = mean(model_durations)
        json_duration = mean(json_durations)
    else:
        model_duration = median(model_durations)
        json_duration = median(json_durations)

    ratio = (model_duration - json_duration) / json_duration
    model_samples = ", ".join(f"{duration:.3f}s" for duration in model_durations)
    json_samples = ", ".join(f"{duration:.3f}s" for duration in json_durations)
    print(
        "[INFO] Performance check: "
        f"stat={stat_method}, "
        f"model-samples=[{model_samples}], "
        f"json-samples=[{json_samples}], "
        f"model-first={model_duration:.3f}s, "
        f"json-first={json_duration:.3f}s, "
        f"regression={ratio * 100:.2f}%, "
        f"threshold={max_regression_ratio * 100:.2f}%"
    )
    if ratio > max_regression_ratio:
        print(
            "[FAILED] Performance regression gate failed: "
            f"model-first regression {ratio * 100:.2f}% exceeds "
            f"threshold {max_regression_ratio * 100:.2f}%."
        )
        return 1
    print("[OK] Performance regression gate passed.")
    return 0
