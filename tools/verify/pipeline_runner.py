#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.services.build_layout import (
    assert_no_legacy_layout,
    resolve_logic_pipeline_root,
    sanitize_segment,
)


def load_toml(path: Path) -> dict:
    try:
        import tomllib

        with path.open("rb") as handle:
            return tomllib.load(handle)
    except ModuleNotFoundError:
        import toml  # type: ignore

        return toml.loads(path.read_text(encoding="utf-8"))


def now_iso() -> str:
    return datetime.now().isoformat(timespec="seconds")


def replace_path(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        if dst.is_dir():
            import shutil

            shutil.rmtree(dst)
        else:
            dst.unlink()
    if src.is_dir():
        import shutil

        shutil.copytree(src, dst)
    else:
        import shutil

        shutil.copy2(src, dst)


def resolve_template(text: str, repo_root: Path, python_exe: str) -> str:
    return (
        text.replace("{repo_root}", repo_root.as_posix())
        .replace("{python}", python_exe)
    )


@dataclass
class StepSpec:
    step_id: str
    name: str
    command: list[str]
    cwd: str
    depends_on: list[str]
    timeout_seconds: int
    retries: int
    env: dict[str, str]
    artifacts: list[str]


def build_step_specs(
    raw_steps: list[dict],
    repo_root: Path,
    python_exe: str,
    default_timeout_seconds: int,
) -> list[StepSpec]:
    steps: list[StepSpec] = []
    for index, raw in enumerate(raw_steps):
        step_id = str(raw.get("id", "")).strip()
        if not step_id:
            raise ValueError(f"steps[{index}] missing id")
        command = raw.get("command")
        if not isinstance(command, list) or not command:
            raise ValueError(f"steps[{index}] command must be non-empty list")
        command_args: list[str] = []
        for item in command:
            if not isinstance(item, str):
                raise ValueError(f"steps[{index}] command args must be string")
            command_args.append(resolve_template(item, repo_root, python_exe))

        raw_cwd = str(raw.get("cwd", "{repo_root}"))
        cwd = resolve_template(raw_cwd, repo_root, python_exe)
        depends_on_raw = raw.get("depends_on", [])
        if not isinstance(depends_on_raw, list):
            raise ValueError(f"steps[{index}] depends_on must be list")
        depends_on = [str(item) for item in depends_on_raw]
        timeout_seconds = int(raw.get("timeout_seconds", default_timeout_seconds))
        retries = int(raw.get("retries", 0))

        env_raw = raw.get("env", {})
        if not isinstance(env_raw, dict):
            raise ValueError(f"steps[{index}] env must be table")
        step_env = {
            str(k): resolve_template(str(v), repo_root, python_exe)
            for k, v in env_raw.items()
        }

        artifacts_raw = raw.get("artifacts", [])
        if not isinstance(artifacts_raw, list):
            raise ValueError(f"steps[{index}] artifacts must be list")
        artifacts = [str(item) for item in artifacts_raw]

        steps.append(
            StepSpec(
                step_id=step_id,
                name=str(raw.get("name", step_id)),
                command=command_args,
                cwd=cwd,
                depends_on=depends_on,
                timeout_seconds=timeout_seconds,
                retries=retries,
                env=step_env,
                artifacts=artifacts,
            )
        )
    return steps


def topo_sort_steps(steps: list[StepSpec]) -> list[StepSpec]:
    id_map = {step.step_id: step for step in steps}
    if len(id_map) != len(steps):
        raise ValueError("duplicate step id found")

    incoming_count = {step.step_id: 0 for step in steps}
    outgoing: dict[str, list[str]] = {step.step_id: [] for step in steps}

    for step in steps:
        for dep in step.depends_on:
            if dep not in id_map:
                raise ValueError(f"step '{step.step_id}' depends on unknown '{dep}'")
            incoming_count[step.step_id] += 1
            outgoing[dep].append(step.step_id)

    queue = [step_id for step_id, count in incoming_count.items() if count == 0]
    sorted_ids: list[str] = []
    while queue:
        current = queue.pop(0)
        sorted_ids.append(current)
        for nxt in outgoing[current]:
            incoming_count[nxt] -= 1
            if incoming_count[nxt] == 0:
                queue.append(nxt)

    if len(sorted_ids) != len(steps):
        raise ValueError("steps dependency graph contains cycle")
    return [id_map[step_id] for step_id in sorted_ids]


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


def run_step(
    step: StepSpec,
    *,
    repo_root: Path,
    pipeline_env: dict[str, str],
    logs_dir: Path,
    step_index: int,
) -> dict:
    step_log_path = logs_dir / f"{step_index:02d}_{sanitize_segment(step.step_id)}.log"
    step_cwd = Path(step.cwd)
    if not step_cwd.is_absolute():
        step_cwd = (repo_root / step_cwd).resolve()

    attempts = max(1, step.retries + 1)
    last_stdout = ""
    last_stderr = ""
    last_return_code = 1
    started_at = now_iso()
    attempt_results: list[dict] = []
    total_elapsed_seconds = 0.0

    for attempt in range(1, attempts + 1):
        env = os.environ.copy()
        env.update(pipeline_env)
        env.update(step.env)
        run_started_perf = time.perf_counter()
        run_started_at = now_iso()
        timed_out = False
        try:
            proc = subprocess.run(
                step.command,
                cwd=str(step_cwd),
                env=env,
                capture_output=True,
                text=True,
                timeout=step.timeout_seconds if step.timeout_seconds > 0 else None,
                check=False,
            )
            last_return_code = proc.returncode
            last_stdout = proc.stdout
            last_stderr = proc.stderr
        except subprocess.TimeoutExpired as exc:
            timed_out = True
            last_return_code = 124
            last_stdout = exc.stdout or ""
            last_stderr = exc.stderr or ""
            if not isinstance(last_stdout, str):
                last_stdout = last_stdout.decode("utf-8", errors="replace")
            if not isinstance(last_stderr, str):
                last_stderr = last_stderr.decode("utf-8", errors="replace")
        elapsed_seconds = time.perf_counter() - run_started_perf
        total_elapsed_seconds += elapsed_seconds
        run_completed_at = now_iso()
        attempt_ok = (last_return_code == 0) and not timed_out
        attempt_results.append(
            {
                "attempt": attempt,
                "started_at": run_started_at,
                "completed_at": run_completed_at,
                "elapsed_seconds": round(elapsed_seconds, 6),
                "return_code": last_return_code,
                "timed_out": timed_out,
                "ok": attempt_ok,
            }
        )
        print(
            f"[STEP] {step.step_id} attempt={attempt}/{attempts} "
            f"return_code={last_return_code} elapsed={elapsed_seconds:.3f}s"
        )
        if attempt_ok:
            break

    completed_at = now_iso()
    ok = last_return_code == 0
    artifact_results = evaluate_artifacts(repo_root, step.artifacts)
    summary = {
        "step_id": step.step_id,
        "name": step.name,
        "command": step.command,
        "cwd": step_cwd.as_posix(),
        "depends_on": step.depends_on,
        "started_at": started_at,
        "completed_at": completed_at,
        "elapsed_seconds": round(total_elapsed_seconds, 6),
        "return_code": last_return_code,
        "ok": ok,
        "attempts": attempt_results,
        "artifacts": artifact_results,
        "log_path": step_log_path.as_posix(),
    }

    step_log_path.parent.mkdir(parents=True, exist_ok=True)
    log_content = [
        f"step_id={step.step_id}",
        f"name={step.name}",
        f"cwd={step_cwd.as_posix()}",
        f"command={' '.join(step.command)}",
        f"started_at={started_at}",
        f"completed_at={completed_at}",
        f"elapsed_seconds={total_elapsed_seconds:.6f}",
        f"return_code={last_return_code}",
        f"ok={ok}",
        "",
        "attempts:",
        json.dumps(attempt_results, ensure_ascii=False, indent=2),
        "",
        "artifacts:",
        json.dumps(artifact_results, ensure_ascii=False, indent=2),
        "",
        "stdout:",
        last_stdout.rstrip(),
        "",
        "stderr:",
        last_stderr.rstrip(),
        "",
    ]
    step_log_path.write_text("\n".join(log_content), encoding="utf-8")
    return summary


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generic TOML-based build/test pipeline runner.",
    )
    parser.add_argument(
        "--config",
        required=True,
        help="TOML pipeline config path.",
    )
    parser.add_argument(
        "--run-id",
        default="",
        help="Optional custom run id.",
    )
    parser.add_argument(
        "--list-steps",
        action="store_true",
        help="List steps in topological order and exit.",
    )
    return parser.parse_args()


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


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    config_path = Path(args.config)
    if not config_path.is_absolute():
        config_path = (repo_root / config_path).resolve()

    if not config_path.exists():
        print(f"[ERROR] Config not found: {config_path}")
        return 2

    config = load_toml(config_path)
    schema_version = int(config.get("schema_version", 1))
    if schema_version != 1:
        print(f"[ERROR] Unsupported schema_version={schema_version}, expected=1")
        return 2

    pipeline_cfg = config.get("pipeline", {})
    if not isinstance(pipeline_cfg, dict):
        print("[ERROR] [pipeline] table is required.")
        return 2
    pipeline_name = str(pipeline_cfg.get("name", "")).strip()
    if not pipeline_name:
        print("[ERROR] [pipeline].name is required.")
        return 2
    default_timeout_seconds = int(pipeline_cfg.get("default_timeout_seconds", 1800))

    output_cfg = config.get("output", {})
    if not isinstance(output_cfg, dict):
        print("[ERROR] [output] table must be table.")
        return 2
    output_root = resolve_output_root(repo_root, output_cfg, pipeline_name)

    env_cfg = config.get("env", {})
    if not isinstance(env_cfg, dict):
        print("[ERROR] [env] must be table.")
        return 2
    pipeline_env = {
        str(k): resolve_template(str(v), repo_root, sys.executable)
        for k, v in env_cfg.items()
    }

    raw_steps = config.get("steps", [])
    if not isinstance(raw_steps, list) or not raw_steps:
        print("[ERROR] [[steps]] list is required and cannot be empty.")
        return 2

    try:
        step_specs = build_step_specs(
            raw_steps=raw_steps,
            repo_root=repo_root,
            python_exe=sys.executable,
            default_timeout_seconds=default_timeout_seconds,
        )
        ordered_steps = topo_sort_steps(step_specs)
    except ValueError as exc:
        print(f"[ERROR] Invalid config: {exc}")
        return 2

    if args.list_steps:
        print(f"pipeline={pipeline_name}")
        for index, step in enumerate(ordered_steps, start=1):
            print(f"{index:02d}. {step.step_id} (depends_on={step.depends_on})")
        return 0

    run_id = args.run_id.strip() or datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    run_dir = output_root / "runs" / sanitize_segment(run_id)
    logs_dir = run_dir / "logs"
    run_dir.mkdir(parents=True, exist_ok=True)

    summary_steps: list[dict] = []
    pipeline_started_at = now_iso()
    failed_step_id = ""
    for index, step in enumerate(ordered_steps, start=1):
        for dep in step.depends_on:
            dep_summary = next((item for item in summary_steps if item["step_id"] == dep), None)
            if dep_summary is None or not dep_summary.get("ok", False):
                failed_step_id = step.step_id
                print(
                    f"[FAILED] Skip step '{step.step_id}' due to dependency failure: {dep}"
                )
                summary_steps.append(
                    {
                        "step_id": step.step_id,
                        "name": step.name,
                        "depends_on": step.depends_on,
                        "ok": False,
                        "skipped": True,
                        "reason": f"dependency_failed:{dep}",
                    }
                )
                break
        else:
            step_summary = run_step(
                step,
                repo_root=repo_root,
                pipeline_env=pipeline_env,
                logs_dir=logs_dir,
                step_index=index,
            )
            summary_steps.append(step_summary)
            if not step_summary["ok"]:
                failed_step_id = step.step_id
                break

    ok = not failed_step_id
    pipeline_completed_at = now_iso()
    summary_payload = {
        "schema_version": schema_version,
        "pipeline": {
            "name": pipeline_name,
            "description": str(pipeline_cfg.get("description", "")),
        },
        "run": {
            "run_id": run_id,
            "config_path": config_path.as_posix(),
            "started_at": pipeline_started_at,
            "completed_at": pipeline_completed_at,
            "ok": ok,
            "failed_step_id": failed_step_id,
            "output_root": output_root.as_posix(),
            "run_dir": run_dir.as_posix(),
        },
        "steps": summary_steps,
    }
    write_json(run_dir / "pipeline_summary.json", summary_payload)
    write_json(
        run_dir / "run_manifest.json",
        {
            "run_id": run_id,
            "pipeline_name": pipeline_name,
            "status": "ok" if ok else "failed",
            "failed_step_id": failed_step_id,
            "config_path": config_path.as_posix(),
            "run_dir": run_dir.as_posix(),
            "completed_at": pipeline_completed_at,
        },
    )
    sync_latest(output_root, run_dir)

    print(
        f"[PIPELINE] name={pipeline_name} run_id={run_id} "
        f"status={'ok' if ok else 'failed'}"
    )
    print(f"[PIPELINE] summary={run_dir / 'pipeline_summary.json'}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
