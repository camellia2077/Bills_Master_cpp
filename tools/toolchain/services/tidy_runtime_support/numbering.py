from __future__ import annotations

from ..tidy_paths import TidyPaths
from ..tidy_queue import load_manifest
from .state_store import load_all_batch_states, load_batch_runtime_state


def build_numbering_context(
    paths: TidyPaths,
    *,
    current_batch_id: str | None,
) -> dict:
    manifest = load_manifest(paths.tasks_manifest)
    pending_batch_numbers = sorted(
        {
            batch_num(str(task.get("batch_id", "")))
            for task in manifest.get("tasks", [])
            if batch_num(str(task.get("batch_id", ""))) > 0
        }
    )
    next_open_batch = batch_id_for_number(pending_batch_numbers[0]) if pending_batch_numbers else None
    effective_current_batch = current_batch_id or next_open_batch
    current_batch_number = batch_num(effective_current_batch or "")
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
        closed_before_current = sorted(inferred_closed_before_current.union(done_batch_numbers))
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


def batch_id_for_number(number: int) -> str:
    return f"batch_{int(number):03d}"


def batch_num(batch_id: str) -> int:
    raw = str(batch_id).strip()
    if raw.startswith("batch_"):
        try:
            return int(raw.replace("batch_", ""))
        except ValueError:
            return 0
    return 0


def _closed_batch_numbers(paths: TidyPaths) -> set[int]:
    batch_numbers: set[int] = set()
    for task_path in paths.tasks_done_dir.rglob("task_*.log"):
        closed_num = batch_num(task_path.parent.name)
        if closed_num > 0:
            batch_numbers.add(closed_num)
    for batch_id, state in load_all_batch_states(paths).items():
        if str(state.get("status", "")).strip() == "done":
            closed_num = batch_num(batch_id)
            if closed_num > 0:
                batch_numbers.add(closed_num)
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
        return batch_id_for_number(start)
    return f"{batch_id_for_number(start)}..{batch_id_for_number(end)}"


def _needs_numbering_refresh(numbering: dict, *, batch_id: str) -> bool:
    if not numbering:
        return True
    current_batch = str(numbering.get("current_batch") or "").strip()
    if current_batch != batch_id:
        return True
    ranges = list(numbering.get("already_closed_ranges", []))
    next_open_batch = str(numbering.get("next_open_batch") or "").strip()
    closed_before_current = int(numbering.get("already_closed_before_current", 0) or 0)
    return not next_open_batch and not ranges and closed_before_current == 0
