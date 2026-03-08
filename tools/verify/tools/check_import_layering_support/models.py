from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class IncludeRecord:
    scope: str
    file_path: Path
    line: int
    delimiter: str
    header: str


@dataclass(frozen=True)
class ImportRecord:
    scope: str
    file_path: Path
    line: int


@dataclass(frozen=True)
class Violation:
    file_path: Path
    line: int
    header: str
    reason: str
