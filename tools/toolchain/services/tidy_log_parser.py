from __future__ import annotations

import re
from collections import defaultdict

TASK_START_PATTERN = re.compile(
    r"^\[(\d+)(?:/(\d+))?\]\s+(?:\[\d+/CHECK\]\s+)?Analyzing:\s+(.+)$"
)
DIAGNOSTIC_LINE_PATTERN = re.compile(
    r"^([A-Za-z]:)?([^:]+):(\d+):(\d+):\s+(warning|error):\s+(.+)$"
)
CHECK_NAME_EXTRACTOR = re.compile(r"(.+)\s+\[([^\]]+)\]$")
ANSI_ESCAPE_PATTERN = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")


def group_sections(log_lines: list[str]) -> list[list[str]]:
    sections: list[list[str]] = []
    current: list[str] = []
    for line in log_lines:
        stripped = _strip_ansi(line).strip()
        if TASK_START_PATTERN.match(stripped):
            if current:
                sections.append(current)
            current = [line]
        else:
            current.append(line)
    if current:
        sections.append(current)
    return sections


def extract_diagnostics(lines: list[str]) -> list[dict]:
    diagnostics: list[dict] = []
    current_diag: dict | None = None
    for line in lines:
        stripped = _strip_ansi(line).strip()
        match = DIAGNOSTIC_LINE_PATTERN.match(stripped)
        if match:
            drive = match.group(1) or ""
            file_path = drive + match.group(2)
            severity = match.group(5)
            full_msg = match.group(6)
            check_match = CHECK_NAME_EXTRACTOR.match(full_msg)
            if check_match:
                message = check_match.group(1)
                check_name = check_match.group(2)
            else:
                message = full_msg
                check_name = f"clang-diagnostic-{severity}"
            current_diag = {
                "file": file_path,
                "line": int(match.group(3)),
                "col": int(match.group(4)),
                "severity": severity,
                "message": message,
                "check": check_name,
                "lines": [line],
            }
            diagnostics.append(current_diag)
        elif current_diag is not None:
            current_diag["lines"].append(line)
    return diagnostics


def generate_text_summary(warnings: list[dict]) -> str:
    if not warnings:
        return ""
    file_counts = defaultdict(int)
    check_counts = defaultdict(int)
    for warning in warnings:
        file_counts[warning["file"].split("/")[-1].split("\\")[-1]] += 1
        check_counts[warning["check"]] += 1
    lines = [
        "=== SUMMARY ===",
        "Files: "
        + ", ".join(
            f"{name}({count})" for name, count in sorted(file_counts.items(), key=lambda item: -item[1])
        ),
        "Types: "
        + ", ".join(
            f"{name}({count})" for name, count in sorted(check_counts.items(), key=lambda item: -item[1])
        ),
        "=" * 15,
        "",
    ]
    return "\n".join(lines)


def strip_task_header(line: str) -> str:
    stripped = _strip_ansi(line).strip()
    match = TASK_START_PATTERN.match(stripped)
    return match.group(3).strip() if match else ""


def _strip_ansi(text: str) -> str:
    return ANSI_ESCAPE_PATTERN.sub("", text)
