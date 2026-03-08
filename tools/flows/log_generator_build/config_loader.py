import sys
from pathlib import Path
from typing import Any


def load_toml(file_path: Path) -> dict[str, Any]:
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


def load_config(config_path: Path) -> dict[str, Any]:
    if not config_path.is_file():
        print(f"!!! Error: Configuration file not found at {config_path}")
        sys.exit(1)

    config = load_toml(config_path)
    if "build" in config:
        print(
            "!!! Error: Legacy [build] section is no longer supported in "
            f"{config_path}. Rename it to [dist]."
        )
        sys.exit(1)
    if not isinstance(config.get("dist"), dict):
        print(f"!!! Error: Missing required [dist] section in {config_path}.")
        sys.exit(1)
    return config
