from __future__ import annotations

from .models import StepSpec


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
