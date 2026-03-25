from __future__ import annotations

from ..timestamps import utc_now_iso

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


def default_phase_map() -> dict:
    return {
        phase: {
            "status": "pending",
            "started_at": None,
            "completed_at": None,
            "updated_at": None,
        }
        for phase in SOP_PHASES
    }


def default_latest_state() -> dict:
    return {
        "generated_at": None,
        "app": "bills-tracer-cli",
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


def default_batch_runtime_state(batch_id: str) -> dict:
    return {
        "generated_at": utc_now_iso(),
        "updated_at": None,
        "batch_id": batch_id,
        "status": "open",
        "current_phase": None,
        "source_files": [],
        "phases": default_phase_map(),
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


def clean_optional_text(value) -> str | None:
    if value is None:
        return None
    text = str(value).strip()
    if not text or text.lower() == "none":
        return None
    return text
