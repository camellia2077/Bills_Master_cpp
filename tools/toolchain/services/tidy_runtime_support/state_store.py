from __future__ import annotations

import json

from ..tidy_paths import TidyPaths
from ..tidy_state import load_json, write_json
from ..timestamps import utc_now_iso
from .defaults import default_batch_runtime_state, default_latest_state, default_phase_map


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
    return load_json(paths.latest_state_path, default_latest_state())


def batch_state_path(paths: TidyPaths, batch_id: str):
    return paths.batch_state_dir / f"{batch_id}.json"


def load_batch_runtime_state(paths: TidyPaths, batch_id: str) -> dict:
    normalized_batch = batch_id.strip()
    return load_json(
        batch_state_path(paths, normalized_batch),
        default_batch_runtime_state(normalized_batch),
    )


def save_batch_runtime_state(paths: TidyPaths, batch_id: str, payload: dict) -> dict:
    normalized_batch = batch_id.strip()
    payload["batch_id"] = normalized_batch
    payload["updated_at"] = utc_now_iso()
    write_json(batch_state_path(paths, normalized_batch), payload)
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
    phases = payload.setdefault("phases", default_phase_map())
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


def load_all_batch_states(paths: TidyPaths) -> dict[str, dict]:
    payload: dict[str, dict] = {}
    for state_path in sorted(paths.batch_state_dir.glob("batch_*.json")):
        batch_id = state_path.stem
        payload[batch_id] = load_batch_runtime_state(paths, batch_id)
    return payload
