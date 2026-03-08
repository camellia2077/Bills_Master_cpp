from __future__ import annotations

from dataclasses import dataclass


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
