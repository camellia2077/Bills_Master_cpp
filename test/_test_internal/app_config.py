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

# --- [核心修改] ---
# 获取当前配置文件所在的目录
_current_dir = os.path.dirname(os.path.abspath(__file__))
# config.toml 与 run_tests.py 同级，位于 _test_internal 上一级目录
_default_config_path = os.path.join(os.path.dirname(_current_dir), "config.toml")
_config_path = os.environ.get("BILLS_MASTER_TEST_CONFIG", "").strip()
if _config_path:
    _config_path = os.path.abspath(_config_path)
else:
    _config_path = _default_config_path

try:
    open_mode = "rb" if _USE_BINARY else "r"
    with open(_config_path, open_mode, encoding=None if _USE_BINARY else "utf-8") as f:
        _data = toml_loader.load(f)
except FileNotFoundError:
    print(f"错误: 配置文件 '{_config_path}' 未找到。")
    sys.exit(1)
except _TOML_DECODE_ERROR as e:
    print(f"错误: 解析 'config.toml' 文件失败: {e}")
    sys.exit(1)

# --- 读取流程控制开关 ---
RUN_CONTROL = _data.get("run_control", {})
RUN_CLEANUP = RUN_CONTROL.get("run_cleanup", True)
RUN_PREPARE_ENV = RUN_CONTROL.get("run_prepare_env", True)
RUN_TESTS = RUN_CONTROL.get("run_tests", True)


# 读取其他配置
PATHS = _data.get("paths", {})
PLUGINS = _data.get("plugins", {})
SETTINGS = _data.get("settings", {})
CLEANUP = _data.get("cleanup", {})
TEST_DATES = _data.get("test_dates", {})

BUILD_DIR = PATHS.get("build_dir")
BILLS_DIR = PATHS.get("bills_dir")
IMPORT_DIR = PATHS.get("import_dir")

PLUGIN_DLLS = PLUGINS.get("plugin_dlls", [])

RUN_EXPORT_ALL_TASKS = SETTINGS.get("run_export_all_tasks", True)
EXPORT_FORMATS = SETTINGS.get("export_formats", [])
INGEST_MODE = SETTINGS.get("ingest_mode", "stepwise")
INGEST_WRITE_JSON = SETTINGS.get("ingest_write_json", False)

FILES_TO_DELETE = CLEANUP.get("files_to_delete", [])
DIRS_TO_DELETE = CLEANUP.get("dirs_to_delete", [])
