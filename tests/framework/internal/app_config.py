# app_config.py

import os
import sys

try:
    import tomllib as toml_loader

    _TOML_DECODE_ERROR = toml_loader.TOMLDecodeError
    _USE_BINARY = True
except ModuleNotFoundError:
    import toml as toml_loader

    _TOML_DECODE_ERROR = toml_loader.TomlDecodeError
    _USE_BINARY = False


_current_dir = os.path.dirname(os.path.abspath(__file__))
_test_root = os.path.dirname(os.path.dirname(_current_dir))
_default_config_path = os.path.join(_test_root, "config", "bills_master.toml")
_config_path = os.environ.get("BILLS_MASTER_TEST_CONFIG", "").strip()
if _config_path:
    _config_path = os.path.abspath(_config_path)
else:
    _config_path = _default_config_path
_config_dir = os.path.dirname(_config_path)

try:
    open_mode = "rb" if _USE_BINARY else "r"
    with open(
        _config_path,
        open_mode,
        encoding=None if _USE_BINARY else "utf-8",
    ) as file:
        _data = toml_loader.load(file)
except FileNotFoundError:
    print(f"Error: config file not found: '{_config_path}'")
    sys.exit(1)
except _TOML_DECODE_ERROR as exc:
    print(f"Error: failed to parse '{_config_path}': {exc}")
    sys.exit(1)


RUN_CONTROL = _data.get("run_control", {})
RUN_CLEANUP = RUN_CONTROL.get("run_cleanup", True)
RUN_PREPARE_ENV = RUN_CONTROL.get("run_prepare_env", True)
RUN_TESTS = RUN_CONTROL.get("run_tests", True)

PATHS = _data.get("paths", {})
SETTINGS = _data.get("settings", {})
CLEANUP = _data.get("cleanup", {})
TEST_DATES = _data.get("test_dates", {})
RUNTIME = _data.get("runtime", {})


def _resolve_config_path(raw_path):
    if not raw_path:
        return raw_path
    _validate_non_legacy_path(raw_path)
    if os.path.isabs(raw_path):
        return os.path.normpath(raw_path)
    return os.path.normpath(os.path.join(_config_dir, raw_path))


def _validate_non_legacy_path(raw_path):
    normalized = str(raw_path).strip().replace("\\", "/")
    for token in (
        "build/",
        "/build/",
        "fixtures/bills",
        "/".join(("tests", "fixtures")),
        "/".join(("tests", "output")),
        "/".join(("test", "output")),
        "_".join(("build", "fast")),
        "_".join(("build", "tidy")),
        "_".join(("build", "debug")),
    ):
        if token in normalized:
            print(
                "Error: legacy pre-dist path is not supported anymore: "
                f"'{raw_path}' (matched '{token}')"
            )
            sys.exit(2)


WORKSPACE_DIR = _resolve_config_path(PATHS.get("workspace_dir"))
BILLS_DIR = _resolve_config_path(PATHS.get("bills_dir"))
IMPORT_DIR = _resolve_config_path(PATHS.get("import_dir"))

_runtime_base_raw = RUNTIME.get("base_dir", "")
RUNTIME_BASE_DIR = _resolve_config_path(_runtime_base_raw) if _runtime_base_raw else ""
RUNTIME_RUN_ID = str(RUNTIME.get("run_id", "")).strip()

_runtime_output_raw = RUNTIME.get("output_dir", "")
RUNTIME_OUTPUT_DIR = _resolve_config_path(_runtime_output_raw) if _runtime_output_raw else ""

_runtime_summary_raw = RUNTIME.get("summary_path", "")
RUNTIME_SUMMARY_PATH = _resolve_config_path(_runtime_summary_raw) if _runtime_summary_raw else ""

RUN_EXPORT_ALL_TASKS = SETTINGS.get("run_export_all_tasks", True)
EXPORT_FORMATS = SETTINGS.get("export_formats", [])
INGEST_MODE = SETTINGS.get("ingest_mode", "stepwise")
INGEST_WRITE_JSON = SETTINGS.get("ingest_write_json", False)
EXPORT_PIPELINE = SETTINGS.get("export_pipeline", "model-first")
OUTPUT_PROJECT = SETTINGS.get("output_project", "bills_tracer")

FILES_TO_DELETE = CLEANUP.get("files_to_delete", [])
DIRS_TO_DELETE = CLEANUP.get("dirs_to_delete", [])
