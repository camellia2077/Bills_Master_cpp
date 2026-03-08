from __future__ import annotations

import json

from ...core.context import Context
from ..tidy_paths import TidyPaths
from ..tidy_queue import load_manifest
from ..tidy_state import write_json
from ..timestamps import utc_now_iso
from .defaults import clean_optional_text, default_batch_runtime_state
from .numbering import build_numbering_context
from .state_store import (
    load_all_batch_states,
    load_batch_runtime_state,
    load_latest_state,
    load_verify_result,
)


def sync_tidy_state(
    ctx: Context,
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    batch_id: str | None = None,
    current_phase: str | None = None,
    next_action: str | None = None,
) -> dict:
    latest = _build_latest_state(
        ctx,
        paths,
        stage=stage,
        status=status,
        exit_code=exit_code,
        batch_id=batch_id,
        current_phase=current_phase,
        next_action_override=next_action,
    )
    write_json(paths.latest_state_path, latest)
    manifest = load_manifest(paths.tasks_manifest)
    pending_tasks = list(manifest.get("tasks", []))
    _write_batch_status_projection(paths, manifest)
    _write_checkpoint_projection(paths, latest)
    _write_tidy_result_projection(ctx, paths, latest, pending_tasks)
    return latest


def write_tidy_result(
    ctx: Context,
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    next_action: str | None = None,
    batch_id: str | None = None,
    current_phase: str | None = None,
) -> None:
    sync_tidy_state(
        ctx,
        paths,
        stage=stage,
        status=status,
        exit_code=exit_code,
        batch_id=batch_id,
        current_phase=current_phase,
        next_action=next_action,
    )


def _build_latest_state(
    ctx: Context,
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    batch_id: str | None,
    current_phase: str | None,
    next_action_override: str | None,
) -> dict:
    manifest = load_manifest(paths.tasks_manifest)
    pending_tasks = list(manifest.get("tasks", []))
    pending_batch_ids = sorted(
        {
            str(task.get("batch_id", "")).strip()
            for task in pending_tasks
            if str(task.get("batch_id", "")).strip()
        },
        key=_batch_num,
    )
    latest_previous = load_latest_state(paths)
    effective_batch_id = (
        clean_optional_text(batch_id)
        or clean_optional_text(latest_previous.get("current_batch"))
        or (pending_batch_ids[0] if pending_batch_ids else None)
    )
    batch_state = (
        load_batch_runtime_state(paths, effective_batch_id)
        if effective_batch_id
        else default_batch_runtime_state("")
    )
    effective_phase = clean_optional_text(current_phase) or clean_optional_text(
        batch_state.get("current_phase")
    )
    numbering_context = build_numbering_context(paths, current_batch_id=effective_batch_id)
    queue = {
        "remaining_tasks": len(pending_tasks),
        "remaining_batches": len(pending_batch_ids),
        "pending_batches": pending_batch_ids,
        "next_open_batch": pending_batch_ids[0] if pending_batch_ids else None,
    }
    verify_payload = load_verify_result(paths)
    return {
        "generated_at": utc_now_iso(),
        "app": "bills",
        "queue": queue,
        "current_batch": effective_batch_id,
        "current_phase": effective_phase,
        "next_action": _resolve_next_action(
            batch_state=batch_state if effective_batch_id else {},
            queue=queue,
            batch_id=effective_batch_id,
            override=next_action_override,
        ),
        "numbering_context": numbering_context,
        "last_verify": {
            "success": bool(verify_payload.get("success", False)),
            "returncode": int(verify_payload.get("returncode", 0)) if verify_payload else None,
            "command": list(verify_payload.get("command", []))
            if isinstance(verify_payload.get("command", []), list)
            else [],
            "updated_at": verify_payload.get("generated_at"),
        },
        "last_run": {
            "stage": stage,
            "status": status,
            "exit_code": int(exit_code),
            "batch_id": effective_batch_id,
            "updated_at": utc_now_iso(),
        },
    }


def _write_batch_status_projection(paths: TidyPaths, manifest: dict) -> None:
    payload = {"version": 2, "batches": {}, "updated_at": utc_now_iso()}
    for batch in manifest.get("batches", []):
        batch_id = str(batch.get("batch_id", "")).strip()
        if not batch_id:
            continue
        payload["batches"][batch_id] = {
            "status": "open",
            "task_count": int(batch.get("task_count", 0) or 0),
            "checks": list(batch.get("checks", [])),
            "updated_at": utc_now_iso(),
        }
    for batch_id, state in load_all_batch_states(paths).items():
        payload["batches"].setdefault(batch_id, {})
        payload["batches"][batch_id].update(
            {
                "status": state.get("status", "open"),
                "stage": state.get("current_phase"),
                "updated_at": state.get("updated_at"),
            }
        )
    write_json(paths.batch_status_path, payload)


def _write_checkpoint_projection(paths: TidyPaths, latest: dict) -> None:
    batch_id = str(latest.get("current_batch", "")).strip()
    if not batch_id:
        return
    payload = {
        "batch_id": batch_id,
        "stage": latest.get("current_phase"),
        "status": _checkpoint_status(paths, batch_id, str(latest.get("current_phase", ""))),
    }
    write_json(paths.checkpoint_path, payload)


def _write_tidy_result_projection(
    ctx: Context,
    paths: TidyPaths,
    latest: dict,
    pending_tasks: list[dict],
) -> None:
    blocking_files = [
        {
            "task_id": task.get("task_id"),
            "batch_id": task.get("batch_id"),
            "source_file": task.get("source_file"),
            "checks": task.get("checks", []),
            "primary_fix_strategy": task.get("primary_fix_strategy"),
            "safe_fix_checks_present": task.get("safe_fix_checks_present", []),
            "suppression_candidates_present": task.get("suppression_candidates_present", []),
        }
        for task in pending_tasks[:20]
    ]
    queue = latest.get("queue", {})
    last_run = latest.get("last_run", {})
    payload = {
        "generated_at": utc_now_iso(),
        "app": "bills",
        "stage": last_run.get("stage", ""),
        "status": last_run.get("status", ""),
        "exit_code": int(last_run.get("exit_code", 0) or 0),
        "tasks": {
            "remaining": int(queue.get("remaining_tasks", 0) or 0),
            "batches": int(queue.get("remaining_batches", 0) or 0),
        },
        "blocking_files": blocking_files,
        "next_action": latest.get("next_action", "No pending tidy tasks."),
        "numbering_context": latest.get("numbering_context", {}),
        "config_snapshot": {
            "max_lines": ctx.config.tidy.max_lines,
            "max_diags": ctx.config.tidy.max_diags,
            "batch_size": ctx.config.tidy.batch_size,
        },
    }
    paths.tidy_result_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def _resolve_next_action(
    *,
    batch_state: dict,
    queue: dict,
    batch_id: str | None,
    override: str | None,
) -> str:
    if override:
        return override
    unexpected_fixable_count = int(
        batch_state.get("decision_summary", {}).get("unexpected_fixable_count", 0) or 0
    )
    if batch_id and unexpected_fixable_count > 0 and _safe_fix_prepass_completed(batch_state):
        return (
            "Workflow bug: safe-fixable checks still remain after recheck for "
            f"{batch_id}. Inspect `python tools/run.py tidy-status --batch-id {batch_id}`."
        )
    batch_status = str(batch_state.get("status", "")).strip()
    if batch_id and batch_status in {"needs_manual", "needs_manual_after_fix"}:
        return f"Next: python tools/run.py tidy-status --batch-id {batch_id}"
    next_open_batch = str(queue.get("next_open_batch", "")).strip()
    if next_open_batch:
        return f"Next: python tools/run.py tidy-batch --batch-id {next_open_batch} --preset sop"
    return "No pending tidy tasks."


def _checkpoint_status(paths: TidyPaths, batch_id: str, phase: str) -> str:
    state = load_batch_runtime_state(paths, batch_id)
    phases = state.get("phases", {})
    phase_entry = phases.get(phase, {}) if isinstance(phases, dict) else {}
    status = str(phase_entry.get("status", "")).strip()
    if status:
        return status
    return str(state.get("status", "open")).strip() or "open"


def _safe_fix_prepass_completed(batch_state: dict) -> bool:
    prepass = batch_state.get("auto_fix_prepass", {})
    return str(prepass.get("status", "")).strip() == "completed"


def _batch_num(batch_id: str) -> int:
    raw = str(batch_id).strip()
    if raw.startswith("batch_"):
        try:
            return int(raw.replace("batch_", ""))
        except ValueError:
            return 0
    return 0
