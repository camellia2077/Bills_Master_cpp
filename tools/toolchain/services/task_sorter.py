from __future__ import annotations

CHECK_WEIGHTS = {
    "readability-identifier-naming": 1,
    "modernize-": 2,
    "readability-": 3,
    "performance-": 5,
    "llvm-": 5,
    "google-": 5,
    "cppcoreguidelines-": 8,
    "bugprone-": 10,
}


def calculate_priority_score(warnings: list[dict], file_path: str) -> float:
    if not warnings:
        return 999.0

    max_check_weight = 5.0
    for warning in warnings:
        check = str(warning.get("check", ""))
        for pattern, weight in CHECK_WEIGHTS.items():
            if check.startswith(pattern):
                max_check_weight = max(max_check_weight, weight)
                break

    file_factor = 2.0 if any(file_path.endswith(ext) for ext in [".hpp", ".h", ".hxx"]) else 1.0
    count_factor = len(warnings) * 0.05
    return (max_check_weight * file_factor) + count_factor
