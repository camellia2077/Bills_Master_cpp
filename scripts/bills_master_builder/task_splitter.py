"""
Split clang-tidy output into individual task log files.
"""

import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple

TASK_START_PATTERN = re.compile(
    r"^\[(\d+)(?:/(\d+))?\]\s+(?:\[\d+/CHECK\]\s+)?Analyzing:\s+(.+)$"
)

DIAGNOSTIC_PATTERN = re.compile(
    r"^([A-Za-z]:)?([^:]+):(\d+):(\d+):\s+(warning|error):\s+(.+?)(?:\s+\[([^\]]+)\])?$"
)

LARGE_TASK_THRESHOLD = 50


def _parse_task_start(line: str) -> Optional[Tuple[Optional[int], str]]:
    stripped = line.strip()
    if "Analyzing:" not in stripped:
        return None

    match = TASK_START_PATTERN.match(stripped)
    if match:
        task_id = int(match.group(1))
        file_path = match.group(3).strip()
        return task_id, file_path

    file_path = stripped.split("Analyzing:", 1)[1].strip()
    return None, file_path


def _extract_warnings(lines: List[str]) -> List[Dict]:
    warnings = []
    for line in lines:
        match = DIAGNOSTIC_PATTERN.match(line.strip())
        if not match:
            continue
        drive = match.group(1) or ""
        file_path = drive + match.group(2)
        severity = match.group(5)
        message = match.group(6)
        check_name = match.group(7) or severity
        warnings.append(
            {
                "file": file_path,
                "line": int(match.group(3)),
                "col": int(match.group(4)),
                "message": message,
                "check": check_name,
            }
        )
    return warnings


def _generate_summary(warnings: List[Dict]) -> str:
    if not warnings:
        return ""

    file_counts: Dict[str, int] = defaultdict(int)
    check_counts: Dict[str, int] = defaultdict(int)

    for warning in warnings:
        filename = Path(warning["file"]).name
        file_counts[filename] += 1
        check_counts[warning["check"]] += 1

    lines = ["=== SUMMARY ==="]
    file_parts = [f"{name}({count})" for name, count in sorted(file_counts.items(), key=lambda item: -item[1])]
    lines.append(f"Files: {', '.join(file_parts)}")
    check_parts = [f"{name}({count})" for name, count in sorted(check_counts.items(), key=lambda item: -item[1])]
    lines.append(f"Types: {', '.join(check_parts)}")
    lines.append("=" * 15)
    lines.append("")
    return "\n".join(lines)


def _group_by_header(lines: List[str], warnings: List[Dict]) -> Dict[str, Tuple[List[str], List[Dict]]]:
    header_groups: Dict[str, List[str]] = defaultdict(list)
    header_warnings: Dict[str, List[Dict]] = defaultdict(list)

    task_start_line = None
    for line in lines:
        if _parse_task_start(line):
            task_start_line = line
            break

    for warning in warnings:
        header_name = Path(warning["file"]).name
        header_warnings[header_name].append(warning)

    for line in lines:
        stripped = line.strip()
        if not stripped or _parse_task_start(stripped):
            continue

        matched_header = None
        for header in header_warnings.keys():
            if header in line:
                matched_header = header
                break

        if matched_header:
            header_groups[matched_header].append(line)
        elif "warning:" in line.lower() or "error:" in line.lower() or "note:" in line.lower():
            pass
        elif "Suppressed" in line or "Use -header-filter" in line:
            for header in header_groups:
                if line not in header_groups[header]:
                    header_groups[header].append(line)

    result = {}
    for header, grouped_warnings in header_warnings.items():
        grouped_lines = header_groups.get(header, [])
        if task_start_line:
            grouped_lines = [task_start_line] + grouped_lines
        result[header] = (grouped_lines, grouped_warnings)

    return result


def _create_task_content(source_file: str, lines: List[str], warnings: List[Dict],
                         sub_header: Optional[str] = None) -> str:
    header = f"File: {source_file}"
    if sub_header:
        header += f" [Sub: {sub_header}]"
    header += "\n" + "=" * 60 + "\n"

    summary = _generate_summary(warnings)
    content = header + summary + "".join(lines)
    return content


def split_tidy_logs(log_lines: List[str], output_dir: Path) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)

    tasks: Dict[int, Dict] = {}
    current_task: Optional[int] = None
    current_file: Optional[str] = None
    current_lines: List[str] = []
    next_task_id = 1

    for line in log_lines:
        parsed = _parse_task_start(line)
        if parsed:
            task_id, file_path = parsed
            if current_task is not None:
                tasks[current_task] = {"file_path": current_file, "lines": current_lines}

            if task_id is None:
                task_id = next_task_id
                next_task_id += 1

            current_task = task_id
            current_file = file_path
            current_lines = [line]
        elif current_task is not None:
            current_lines.append(line)

    if current_task is not None:
        tasks[current_task] = {"file_path": current_file, "lines": current_lines}

    final_tasks = []
    for task_data in tasks.values():
        lines = task_data["lines"]
        warnings = _extract_warnings(lines)
        if not warnings:
            continue

        line_count = len(lines)
        source_file = task_data["file_path"]
        unique_headers = set(Path(warning["file"]).name for warning in warnings)

        if line_count > LARGE_TASK_THRESHOLD and len(unique_headers) > 1:
            header_groups = _group_by_header(lines, warnings)
            for header_name, (header_lines, header_warnings) in header_groups.items():
                if header_warnings:
                    content = _create_task_content(source_file, header_lines, header_warnings, header_name)
                    final_tasks.append({"content": content, "size": len(content)})
        else:
            content = _create_task_content(source_file, lines, warnings)
            final_tasks.append({"content": content, "size": len(content)})

    final_tasks.sort(key=lambda item: item["size"])

    created_count = 0
    for idx, task_data in enumerate(final_tasks, start=1):
        task_id = f"{idx:03d}"
        task_file = output_dir / f"task_{task_id}.log"
        task_file.write_text(task_data["content"], encoding="utf-8")
        created_count += 1

    return created_count
