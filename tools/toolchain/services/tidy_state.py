from __future__ import annotations

import json
from pathlib import Path

from .timestamps import utc_now_iso


def load_json(path: Path, default: dict) -> dict:
    if not path.exists():
        return dict(default)
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return dict(default)
    if not isinstance(payload, dict):
        return dict(default)
    merged = dict(default)
    merged.update(payload)
    return merged


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def load_batch_state(path: Path) -> dict:
    return load_json(
        path,
        {
            "version": 1,
            "batch_id": None,
            "cleaned_task_ids": [],
            "last_verify_success": None,
            "last_refresh_ok": None,
            "updated_at": None,
        },
    )


def update_batch_state(path: Path, **fields) -> dict:
    state = load_batch_state(path)
    state.update(fields)
    state["updated_at"] = utc_now_iso()
    write_json(path, state)
    return state


def load_batch_status(path: Path) -> dict:
    return load_json(path, {"version": 1, "batches": {}, "updated_at": None})


def set_batch_status(path: Path, batch_id: str, status: str, **extra_fields) -> dict:
    payload = load_batch_status(path)
    batches = payload.setdefault("batches", {})
    entry = batches.setdefault(batch_id, {})
    entry["status"] = status
    entry.update(extra_fields)
    entry["updated_at"] = utc_now_iso()
    payload["updated_at"] = utc_now_iso()
    write_json(path, payload)
    return payload


def load_refresh_state(path: Path) -> dict:
    return load_json(
        path,
        {
            "version": 1,
            "processed_batches": [],
            "batches_since_full": 0,
            "last_batch": None,
            "last_full_reason": "",
            "last_full_batch": None,
            "last_full_at": None,
            "updated_at": None,
        },
    )


def update_refresh_state(path: Path, payload: dict) -> dict:
    payload["updated_at"] = utc_now_iso()
    write_json(path, payload)
    return payload
