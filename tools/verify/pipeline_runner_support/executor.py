from __future__ import annotations

import json
import os
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

from tools.toolchain.services.build_layout import sanitize_segment

from .artifacts import evaluate_artifacts, resolve_output_root, sync_latest, write_json
from .config_loader import build_step_specs, load_toml, resolve_template
from .dag import topo_sort_steps
from .models import StepSpec


def now_iso() -> str:
    return datetime.now().isoformat(timespec="seconds")


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


def run_pipeline(args) -> int:
    repo_root = Path(__file__).resolve().parents[3]
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
        str(k): resolve_template(str(v), repo_root, sys.executable) for k, v in env_cfg.items()
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
                print(f"[FAILED] Skip step '{step.step_id}' due to dependency failure: {dep}")
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

    print(f"[PIPELINE] name={pipeline_name} run_id={run_id} status={'ok' if ok else 'failed'}")
    print(f"[PIPELINE] summary={run_dir / 'pipeline_summary.json'}")
    return 0 if ok else 1
