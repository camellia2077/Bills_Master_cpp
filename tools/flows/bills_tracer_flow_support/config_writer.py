from __future__ import annotations

from pathlib import Path

FORMAT_CONFIG = {
    "json": {
        "cmake_option": "",
    },
    "md": {
        "cmake_option": "ENABLE_FMT_MD",
    },
    "rst": {
        "cmake_option": "ENABLE_FMT_RST",
    },
    "tex": {
        "cmake_option": "ENABLE_FMT_TEX",
    },
}
RUNTIME_EXPORT_FORMATS_FILENAME = "export_formats.toml"
CLEANUP_FILES = ["bills.sqlite3"]
CLEANUP_DIRS = ["dist", "config", "output"]


def parse_formats(raw_formats: str) -> list[str]:
    formats = [item.strip().lower() for item in raw_formats.split(",") if item.strip()]
    if not formats:
        raise ValueError("No valid format found in --formats.")
    unsupported = [fmt for fmt in formats if fmt not in FORMAT_CONFIG]
    if unsupported:
        raise ValueError(
            f"Unsupported format(s): {', '.join(unsupported)}. "
            f"Supported: {', '.join(FORMAT_CONFIG.keys())}."
        )
    deduped: list[str] = []
    for fmt in formats:
        if fmt not in deduped:
            deduped.append(fmt)
    return deduped


def cmake_format_defines(formats: list[str]) -> list[str]:
    enabled = set(formats)
    defines: list[str] = []
    for fmt, cfg in FORMAT_CONFIG.items():
        option = cfg["cmake_option"]
        if not option:
            continue
        defines.append(f"-D{option}={'ON' if fmt in enabled else 'OFF'}")
    return defines


def write_runtime_export_formats_config(build_bin_dir: Path, export_formats: list[str]) -> None:
    config_dir = build_bin_dir / "config"
    config_dir.mkdir(parents=True, exist_ok=True)
    config_path = config_dir / RUNTIME_EXPORT_FORMATS_FILENAME
    config_path.write_text(
        f"enabled_formats = {to_toml_list(export_formats)}\n",
        encoding="utf-8",
    )


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(f"'{item}'" for item in items) + "]"


def build_cleanup_dirs() -> list[str]:
    return [*CLEANUP_DIRS]


def write_temp_test_config(
    config_path: Path,
    workspace_dir: Path,
    bills_dir: Path,
    import_dir: Path,
    runtime_base_dir: Path,
    runtime_run_id: str,
    runtime_output_dir: Path,
    runtime_summary_path: Path,
    run_export_all_tasks: bool,
    export_formats: list[str],
    ingest_mode: str,
    ingest_write_json: bool,
    export_pipeline: str,
    output_project: str,
    single_year: str,
    single_month: str,
    range_start: str,
    range_end: str,
) -> None:
    config_text = f"""[paths]
workspace_dir = '{workspace_dir.as_posix()}'
bills_dir = '{bills_dir.as_posix()}'
import_dir = '{import_dir.as_posix()}'

[runtime]
base_dir = '{runtime_base_dir.as_posix()}'
run_id = '{runtime_run_id}'
output_dir = '{runtime_output_dir.as_posix()}'
summary_path = '{runtime_summary_path.as_posix()}'

[run_control]
run_cleanup = true
run_prepare_env = true
run_tests = true

[settings]
run_export_all_tasks = {str(run_export_all_tasks).lower()}
ingest_mode = '{ingest_mode}'
ingest_write_json = {str(ingest_write_json).lower()}
export_pipeline = '{export_pipeline}'
export_formats = {to_toml_list(export_formats)}
output_project = '{output_project}'

[cleanup]
files_to_delete = {to_toml_list(CLEANUP_FILES)}
dirs_to_delete = {to_toml_list(build_cleanup_dirs())}

[test_dates]
single_year = '{single_year}'
single_month = '{single_month}'
range_start = '{range_start}'
range_end = '{range_end}'
"""
    config_path.write_text(config_text, encoding="utf-8")
