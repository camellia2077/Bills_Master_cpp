import sys
from pathlib import Path
from typing import Any, Dict

def load_toml(file_path: Path) -> Dict[str, Any]:
    """
    Load TOML file with fallback mechanism:
    1. standard library 'tomllib' (Python 3.11+)
    2. external library 'toml'
    """
    try:
        import tomllib
        with open(file_path, "rb") as f:
            return tomllib.load(f)
    except ImportError:
        try:
            import toml
            return toml.load(file_path)
        except ImportError:
            print("!!! Error: No TOML parser found.")
            print("!!! Please use Python 3.11+ (built-in 'tomllib') or install 'toml' package (pip install toml).")
            sys.exit(1)
    except Exception as e:
        print(f"!!! Error loading config file {file_path}: {e}")
        sys.exit(1)

def load_config() -> Dict[str, Any]:
    """
    Load configuration from 'script/config.toml'.
    """
    # Assuming this script is running from the 'script' directory or accessed via module
    # We need to find config.toml relative to this file
    # This file is in script/automation/config_loader.py
    # config.toml is in script/config.toml
    
    current_file = Path(__file__).resolve()
    script_dir = current_file.parent.parent # script/automation -> script
    config_path = script_dir / "config.toml"
    
    if not config_path.is_file():
        print(f"!!! Error: Configuration file not found at {config_path}")
        sys.exit(1)
        
    return load_toml(config_path)

# Expose a simple interface to access config values like object attributes for backward compatibility if needed,
# or just return the dict. Returning dict is cleaner for new code.
