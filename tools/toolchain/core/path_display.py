from __future__ import annotations

from pathlib import Path


def display_path(path: Path | str, *, resolve: bool = False) -> str:
    candidate = Path(path)
    if resolve:
        candidate = candidate.resolve()
    return str(candidate).replace("/", "\\")


def display_path_from_repo(repo_root: Path, path: Path | str) -> str:
    if not str(path).strip():
        return ""
    candidate = Path(path)
    if not candidate.is_absolute():
        candidate = repo_root / candidate
    return display_path(candidate, resolve=True)


def display_command(command: list[str]) -> str:
    return " ".join(_display_command_arg(arg) for arg in command)


def _display_command_arg(arg: str) -> str:
    if not _looks_like_path(arg):
        return arg
    if arg.startswith("-") and "=" in arg:
        prefix, value = arg.split("=", 1)
        return f"{prefix}={display_path(value)}"
    return display_path(arg)


def _looks_like_path(value: str) -> bool:
    if not value or value == "--":
        return False
    if any(token in value for token in ("[\\/]", ".*(", "glob", "*", "?")):
        return False
    if value.startswith("-") and "=" not in value:
        return False
    if _looks_like_windows_absolute(value):
        return True
    if "\\" in value or "/" in value:
        return True
    return False


def _looks_like_windows_absolute(value: str) -> bool:
    return len(value) >= 3 and value[1] == ":" and value[2] in ("\\", "/")
