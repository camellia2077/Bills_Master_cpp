# config_handler.py

import config

class AppConfig:
    """A simple class to hold application configuration."""
    def __init__(self, db_path, font_list):
        self.db_path = db_path
        self.font_list = font_list

def load_and_validate_config():
    """
    Loads configuration from config.py and validates it.

    Raises:
        ValueError: If the database path in the config is empty.
    
    Returns:
        AppConfig: An object containing the validated configuration.
    """
    db_path = config.DATABASE_PATH
    font_list = config.FONT_LIST

    if not db_path or not db_path.strip():
        raise ValueError("Error: DATABASE_PATH in config.py cannot be empty.")
    
    print("Configuration loaded and validated successfully.")
    return AppConfig(db_path, font_list)