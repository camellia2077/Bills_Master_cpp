# chart_generator/config/handler.py

import tomllib
from pathlib import Path


# [核心修改] AppConfig 类保持不变
class AppConfig:
    """A simple class to hold application configuration."""

    def __init__(self, db_path, font_list):
        self.db_path = db_path
        self.font_list = font_list


# [核心修改] 重写配置加载函数以使用 TOML
def load_and_validate_config():
    """
    从项目根目录的 config.toml 文件加载并验证配置。

    Raises:
        FileNotFoundError: 如果 config.toml 文件未找到。
        ValueError: 如果数据库路径为空。

    Returns:
        AppConfig: 一个包含已验证配置的对象。
    """
    config_path = Path(__file__).resolve().parents[1] / "config.toml"
    try:
        with config_path.open("rb") as f:
            config_data = tomllib.load(f)
    except FileNotFoundError as err:
        raise FileNotFoundError(f"错误: 未找到配置文件: {config_path}") from err

    # 从解析后的 TOML 数据中提取配置
    db_path_value = config_data.get("database", {}).get("path")
    font_list = config_data.get("visualization", {}).get("font_list", [])

    if not db_path_value or not str(db_path_value).strip():
        raise ValueError("错误: 在 config.toml 中 [database] -> 'path' 的值不能为空。")

    db_path = Path(str(db_path_value).strip())
    if not db_path.is_absolute():
        db_path = (config_path.parent / db_path).resolve()

    print(f"Configuration loaded and validated successfully from {config_path.name}.")
    return AppConfig(str(db_path), font_list)
