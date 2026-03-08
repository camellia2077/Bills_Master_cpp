from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from ...core.context import Context
from ...services.tidy_paths import TidyPaths


@dataclass(frozen=True)
class BatchExecutionRequest:
    batch_id: str
    strict_clean: bool = False
    run_verify: bool = False
    full_every: int | None = None
    keep_going: bool | None = None
    concise: bool = False
    timeout_seconds: int | None = None


@dataclass(frozen=True)
class StepOutcome:
    stop: bool = False
    return_code: int = 0


@dataclass
class BatchExecutionState:
    ctx: Context
    paths: TidyPaths
    request: BatchExecutionRequest
    started: float
    normalized_batch: str
    batch_tasks: list[dict]
    effective_full_every: int
    source_files: list[Path]
    matched_safe_fix_checks: list[str]
    fix_result: Any | None = None
    recheck_result: Any | None = None
    clean_result: Any | None = None
