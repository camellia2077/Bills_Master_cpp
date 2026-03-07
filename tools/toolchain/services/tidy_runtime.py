from __future__ import annotations

import json
import shutil
from pathlib import Path

from ..core.context import Context
from .tidy_log_parser import extract_diagnostics, group_sections
from .tidy_paths import TidyPaths
from .tidy_queue import load_manifest
from .tidy_state import load_json, write_json
from .timestamps import utc_now_iso

SOP_PHASES = (
    "load_queue",
    "verify",
    "safe_fix_prepass",
    "build_gate",
    "recheck",
    "classify",
    "clean_refresh",
    "finalize",
)


def write_latest_tidy_summary(
    paths: TidyPaths,
    *,
    stage: str,
    status: str,
    exit_code: int,
    extra: dict | None = None,
) -> None:
    payload = {
        "generated_at": utc_now_iso(),
        "stage": stage,
        "status": status,
        "exit_code": int(exit_code),
    }
    if extra:
        payload.update(extra)
    paths.latest_summary.parent.mkdir(parents=True, exist_ok=True)
    paths.latest_summary.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def copy_build_log_to_raw(paths: TidyPaths) -> bool:
    if not paths.build_log_path.exists():
        return False
    paths.latest_build_log.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.build_log_path, paths.latest_build_log)
    return True


def new_tidy_run_dir(paths: TidyPaths) -> Path:
    run_dir = paths.run_root / utc_now_iso().replace(":", "").replace("-", "")
    (run_dir / "raw").mkdir(parents=True, exist_ok=True)
    return run_dir


def copy_build_log_to_run(paths: TidyPaths, run_dir: Path) -> Path | None:
    if not paths.build_log_path.exists():
        return None
    destination = run_dir / "raw" / "build.log"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.build_log_path, destination)
    return destination


def write_diagnostics_jsonl(
    *,
    log_content: str,
    output_path: Path,
) -> int:
    diagnostics: list[dict] = []
    for section in group_sections(log_content.splitlines()):
        diagnostics.extend(extract_diagnostics(section))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as handle:
        for item in diagnostics:
            handle.write(json.dumps(item, ensure_ascii=False) + "\n")
    return len(diagnostics)


def copy_compile_commands_to_run(paths: TidyPaths, run_dir: Path) -> Path | None:
    if not paths.compile_commands_path.exists():
        return None
    destination = run_dir / "raw" / "compile_commands.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.compile_commands_path, destination)
    return destination


def write_verify_result(paths: TidyPaths, *, command: list[str], returncode: int) -> None:
    payload = {
        "generated_at": utc_now_iso(),
        "success": returncode == 0,
        "returncode": int(returncode),
        "command": command,
    }
    paths.verify_result_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def latest_verify_succeeded(paths: TidyPaths) -> tuple[bool, str]:
    payload = load_verify_result(paths)
    if not payload:
        return False, "missing verify result"
    if bool(payload.get("success")):
        return True, "ok"
    return False, "latest verify returned failure"


def latest_verify_result_mtime(paths: TidyPaths) -> float | None:
    if not paths.verify_result_path.exists():
        return None
    try:
        return paths.verify_result_path.stat().st_mtime
    except OSError:
        return None


def load_verify_result(paths: TidyPaths) -> dict:
    return load_json(paths.verify_result_path, {})


def load_latest_state(paths: TidyPaths) -> dict:
    return load_json(paths.latest_state_path, _default_latest_state())


def load_batch_runtime_state(paths: TidyPaths, batch_id: str) -> dict:
    normalized_batch = batch_id.strip()
    return load_json(
        _batch_state_path(paths, normalized_batch),
        _default_batch_runtime_state(normalized_batch),
    )


def save_batch_runtime_state(paths: TidyPaths, batch_id: str, payload: dict) -> dict:
    normalized_batch = batch_id.strip()
    payload["batch_id"] = normalized_batch
    payload["updated_at"] = utc_now_iso()
    write_json(_batch_state_path(paths, normalized_batch), payload)
    return payload


def update_batch_runtime_state(paths: TidyPaths, batch_id: str, **fields) -> dict:
    payload = load_batch_runtime_state(paths, batch_id)
    payload.update(fields)
    return save_batch_runtime_state(paths, batch_id, payload)


def update_batch_phase(
    paths: TidyPaths,
    *,
    batch_id: str,
    phase: str,
    status: str,
    details: dict | None = None,
    advance_current_phase: bool = True,
) -> dict:
    payload = load_batch_runtime_state(paths, batch_id)
    phases = payload.setdefault("phases", _default_phase_map())
    phase_entry = dict(phases.get(phase, {"status": "pending"}))
    now = utc_now_iso()
    if phase_entry.get("started_at") is None and status == "running":
        phase_entry["started_at"] = now
    if status not in {"pending", "running"}:
        phase_entry["completed_at"] = now
    phase_entry["status"] = status
    phase_entry["updated_at"] = now
    if details:
        phase_entry.update(details)
    phases[phase] = phase_entry
    if advance_current_phase:
        payload["current_phase"] = phase
    return save_batch_runtime_state(paths, batch_id, payload)


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


def build_numbering_context(
    paths: TidyPaths,
    *,
    current_batch_id: str | None,
) -> dict:
    manifest = load_manifest(paths.tasks_manifest)
    pending_batch_numbers = sorted(
        {
            _batch_num(str(task.get("batch_id", "")))
            for task in manifest.get("tasks", [])
            if _batch_num(str(task.get("batch_id", ""))) > 0
        }
    )
    next_open_batch = (
        _batch_id(pending_batch_numbers[0]) if pending_batch_numbers else None
    )
    effective_current_batch = current_batch_id or next_open_batch
    current_batch_number = _batch_num(effective_current_batch or "")
    done_batch_numbers = sorted(_closed_batch_numbers(paths))
    if current_batch_number > 0:
        pending_before_current = {
            number for number in pending_batch_numbers if number < current_batch_number
        }
        inferred_closed_before_current = {
            number
            for number in range(1, current_batch_number)
            if number not in pending_before_current
        }
        closed_before_current = sorted(
            inferred_closed_before_current.union(done_batch_numbers)
        )
    else:
        closed_before_current = done_batch_numbers
    return {
        "current_batch": effective_current_batch,
        "already_closed_before_current": len(closed_before_current),
        "already_closed_ranges": _format_batch_ranges(closed_before_current),
        "next_open_batch": next_open_batch,
    }


def resolve_batch_numbering_context(
    paths: TidyPaths,
    *,
    batch_id: str,
    batch_state: dict | None = None,
) -> dict:
    state = batch_state or load_batch_runtime_state(paths, batch_id)
    numbering = state.get("numbering_context", {})
    if _needs_numbering_refresh(numbering, batch_id=batch_id):
        return build_numbering_context(paths, current_batch_id=batch_id)
    return numbering


def empty_build_gate_state(*, status: str = "not_run") -> dict:
    return {
        "status": status,
        "command": [],
        "returncode": None,
        "suspect_files": [],
    }


def empty_recheck_state(*, status: str = "not_run") -> dict:
    return {
        "status": status,
        "files": [],
        "log_path": None,
        "diagnostics_path": None,
        "returncode": None,
        "diagnostics_count": 0,
    }


def empty_remaining_state() -> dict:
    return {
        "count": 0,
        "diagnostics": [],
    }


def empty_decision_summary() -> dict:
    return {
        "manual_refactor_count": 0,
        "suggest_nolint_count": 0,
        "unexpected_fixable_count": 0,
        "files_with_remaining": [],
    }


def resolve_batch_source_files(
    ctx: Context,
    paths: TidyPaths,
    *,
    batch_id: str,
) -> list[Path]:
    normalized_batch = batch_id.strip()
    manifest = load_manifest(paths.tasks_manifest)
    files: list[Path] = []
    seen: set[str] = set()
    for task in manifest.get("tasks", []):
        if str(task.get("batch_id", "")).strip() != normalized_batch:
            continue
        raw = str(task.get("source_file", "")).strip()
        if not raw:
            continue
        path = _resolve_source_path(ctx, raw)
        key = _path_key(path)
        if key in seen:
            continue
        seen.add(key)
        files.append(path)
    if files:
        return files

    batch_state = load_batch_runtime_state(paths, normalized_batch)
    for raw in batch_state.get("source_files", []):
        path = _resolve_source_path(ctx, str(raw))
        key = _path_key(path)
        if key in seen:
            continue
        seen.add(key)
        files.append(path)
    return files


def extract_task_source_files(task_path: Path) -> list[Path]:
    files: list[Path] = []
    try:
        lines = task_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return files
    for line in lines:
        if not line.startswith("File: "):
            continue
        raw = line[len("File: ") :].strip()
        if not raw:
            continue
        path = Path(raw)
        if path not in files:
            files.append(path)
    return files


def load_pending_tasks_with_content(tasks_root: Path, manifest_path: Path) -> list[dict]:
    manifest = load_manifest(manifest_path)
    tasks: list[dict] = []
    for item in manifest.get("tasks", []):
        content_path = tasks_root.parent / str(item.get("content_path", ""))
        if not content_path.exists():
            continue
        try:
            content = content_path.read_text(encoding="utf-8")
        except OSError:
            continue
        tasks.append(
            {
                "task_id": str(item.get("task_id", "")),
                "batch_id": str(item.get("batch_id", "")),
                "content": content,
                "size": int(item.get("size", len(content))),
                "diagnostics": [{}] * int(item.get("diagnostic_count", 0)),
                "source_file": str(item.get("source_file", "")),
                "checks": list(item.get("checks", [])),
                "primary_fix_strategy": str(
                    item.get("primary_fix_strategy", "manual_only")
                ),
                "safe_fix_checks_present": list(item.get("safe_fix_checks_present", [])),
                "suppression_candidates_present": list(
                    item.get("suppression_candidates_present", [])
                ),
                "score": float(item.get("score", 999.0)),
            }
        )
    return tasks


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
        _clean_optional_text(batch_id)
        or _clean_optional_text(latest_previous.get("current_batch"))
        or (pending_batch_ids[0] if pending_batch_ids else None)
    )
    batch_state = (
        load_batch_runtime_state(paths, effective_batch_id)
        if effective_batch_id
        else _default_batch_runtime_state("")
    )
    effective_phase = _clean_optional_text(current_phase) or _clean_optional_text(
        batch_state.get("current_phase")
    )
    numbering_context = build_numbering_context(
        paths,
        current_batch_id=effective_batch_id,
    )
    queue = {
        "remaining_tasks": len(pending_tasks),
        "remaining_batches": len(pending_batch_ids),
        "pending_batches": pending_batch_ids,
        "next_open_batch": pending_batch_ids[0] if pending_batch_ids else None,
    }
    verify_payload = load_verify_result(paths)
    latest = {
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
            "returncode": int(verify_payload.get("returncode", 0))
            if verify_payload
            else None,
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
    return latest


def _default_latest_state() -> dict:
    return {
        "generated_at": None,
        "app": "bills",
        "queue": {
            "remaining_tasks": 0,
            "remaining_batches": 0,
            "pending_batches": [],
            "next_open_batch": None,
        },
        "current_batch": None,
        "current_phase": None,
        "next_action": "No pending tidy tasks.",
        "numbering_context": {
            "current_batch": None,
            "already_closed_before_current": 0,
            "already_closed_ranges": [],
            "next_open_batch": None,
        },
        "last_verify": {
            "success": False,
            "returncode": None,
            "command": [],
            "updated_at": None,
        },
        "last_run": {
            "stage": "",
            "status": "",
            "exit_code": 0,
            "batch_id": None,
            "updated_at": None,
        },
    }


def _default_batch_runtime_state(batch_id: str) -> dict:
    return {
        "generated_at": utc_now_iso(),
        "updated_at": None,
        "batch_id": batch_id,
        "status": "open",
        "current_phase": None,
        "source_files": [],
        "phases": _default_phase_map(),
        "auto_fix_prepass": {
            "status": "not_run",
            "checks": [],
            "matched_checks_present": [],
            "target_files": [],
            "changed_files": [],
            "log_path": None,
            "returncode": None,
        },
        "build_gate": empty_build_gate_state(),
        "recheck": empty_recheck_state(),
        "remaining": empty_remaining_state(),
        "decision_summary": empty_decision_summary(),
        "numbering_context": {
            "current_batch": batch_id,
            "already_closed_before_current": 0,
            "already_closed_ranges": [],
            "next_open_batch": None,
        },
    }


def _default_phase_map() -> dict:
    return {
        phase: {
            "status": "pending",
            "started_at": None,
            "completed_at": None,
            "updated_at": None,
        }
        for phase in SOP_PHASES
    }


def _batch_state_path(paths: TidyPaths, batch_id: str) -> Path:
    return paths.batch_state_dir / f"{batch_id}.json"


def _load_all_batch_states(paths: TidyPaths) -> dict[str, dict]:
    payload: dict[str, dict] = {}
    for state_path in sorted(paths.batch_state_dir.glob("batch_*.json")):
        batch_id = state_path.stem
        payload[batch_id] = load_batch_runtime_state(paths, batch_id)
    return payload


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
    for batch_id, state in _load_all_batch_states(paths).items():
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
            "suppression_candidates_present": task.get(
                "suppression_candidates_present", []
            ),
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
    if (
        batch_id
        and unexpected_fixable_count > 0
        and _safe_fix_prepass_completed(batch_state)
    ):
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


def _closed_batch_numbers(paths: TidyPaths) -> set[int]:
    batch_numbers: set[int] = set()
    for task_path in paths.tasks_done_dir.rglob("task_*.log"):
        batch_num = _batch_num(task_path.parent.name)
        if batch_num > 0:
            batch_numbers.add(batch_num)
    for batch_id, state in _load_all_batch_states(paths).items():
        if str(state.get("status", "")).strip() == "done":
            batch_num = _batch_num(batch_id)
            if batch_num > 0:
                batch_numbers.add(batch_num)
    return batch_numbers


def _format_batch_ranges(batch_numbers: list[int]) -> list[str]:
    if not batch_numbers:
        return []
    sorted_numbers = sorted(set(number for number in batch_numbers if number > 0))
    if not sorted_numbers:
        return []
    ranges: list[str] = []
    range_start = sorted_numbers[0]
    previous = sorted_numbers[0]
    for number in sorted_numbers[1:]:
        if number == previous + 1:
            previous = number
            continue
        ranges.append(_range_text(range_start, previous))
        range_start = number
        previous = number
    ranges.append(_range_text(range_start, previous))
    return ranges


def _range_text(start: int, end: int) -> str:
    if start == end:
        return _batch_id(start)
    return f"{_batch_id(start)}..{_batch_id(end)}"


def _batch_id(number: int) -> str:
    return f"batch_{int(number):03d}"


def _batch_num(batch_id: str) -> int:
    raw = str(batch_id).strip()
    if raw.startswith("batch_"):
        try:
            return int(raw.replace("batch_", ""))
        except ValueError:
            return 0
    return 0


def _needs_numbering_refresh(numbering: dict, *, batch_id: str) -> bool:
    if not numbering:
        return True
    current_batch = str(numbering.get("current_batch") or "").strip()
    if current_batch != batch_id:
        return True
    ranges = list(numbering.get("already_closed_ranges", []))
    next_open_batch = str(numbering.get("next_open_batch") or "").strip()
    closed_before_current = int(
        numbering.get("already_closed_before_current", 0) or 0
    )
    return not next_open_batch and not ranges and closed_before_current == 0


def _resolve_source_path(ctx: Context, raw: str) -> Path:
    candidate = Path(raw)
    if candidate.is_absolute():
        return candidate
    return (ctx.repo_root / candidate).resolve()


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()


def _clean_optional_text(value) -> str | None:
    if value is None:
        return None
    text = str(value).strip()
    if not text or text.lower() == "none":
        return None
    return text
