# app_config.py

import os
import sys
import tomllib

# --- [核心修改] ---
# 获取当前配置文件所在的目录
_current_dir = os.path.dirname(os.path.abspath(__file__))
# 构建 config.toml 的完整路径
_config_path = os.path.join(_current_dir, "config.toml")

try:
    with open(_config_path, "rb") as f:
        _data = tomllib.load(f)
except FileNotFoundError:
    print(f"错误: 配置文件 '{_config_path}' 未找到。")
    sys.exit(1)
except tomllib.TOMLDecodeError as e:
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

FILES_TO_DELETE = CLEANUP.get("files_to_delete", [])
DIRS_TO_DELETE = CLEANUP.get("dirs_to_delete", [])