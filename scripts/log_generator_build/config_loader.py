import sys
from pathlib import Path
from typing import Any, Dict


def load_toml(file_path: Path) -> Dict[str, Any]:
    try:
        import tomllib

        with open(file_path, "rb") as file:
            return tomllib.load(file)
    except ImportError:
        try:
            import toml

            return toml.load(file_path)
        except ImportError:
            print("!!! Error: No TOML parser found.")
            print(
                "!!! Please use Python 3.11+ (built-in 'tomllib') "
                "or install 'toml' package (pip install toml)."
            )
            sys.exit(1)
    except Exception as exc:
        print(f"!!! Error loading config file {file_path}: {exc}")
        sys.exit(1)


def load_config(config_path: Path) -> Dict[str, Any]:
    if not config_path.is_file():
        print(f"!!! Error: Configuration file not found at {config_path}")
        sys.exit(1)

    return load_toml(config_path)
