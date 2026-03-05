# chart_generator/config/handler.py

import tomllib

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
    try:
        with open("config.toml", "rb") as f:
            config_data = tomllib.load(f)
    except FileNotFoundError:
        raise FileNotFoundError("错误: 未在项目根目录找到 config.toml 配置文件。")

    # 从解析后的 TOML 数据中提取配置
    db_path = config_data.get("database", {}).get("path")
    font_list = config_data.get("visualization", {}).get("font_list", [])

    if not db_path or not db_path.strip():
        raise ValueError("错误: 在 config.toml 中 [database] -> 'path' 的值不能为空。")
    
    print("Configuration loaded and validated successfully from config.toml.")
    return AppConfig(db_path, font_list)