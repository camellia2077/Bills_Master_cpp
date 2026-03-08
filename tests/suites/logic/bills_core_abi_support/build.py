from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from tools.toolchain.core.config import load_toolchain_config
from tools.toolchain.services.build_layout import resolve_build_directory

REPO_ROOT = Path(__file__).resolve().parents[4]
WORKFLOW_TOML_PATH = REPO_ROOT / "tools" / "toolchain" / "config" / "workflow.toml"


def run_build(build_preset: str) -> None:
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "flows" / "build_bills_core.py"),
        "--preset",
        build_preset,
        "--shared",
    ]
    print(f"==> Running: {' '.join(command)}")
    subprocess.run(command, check=True, cwd=REPO_ROOT)


def detect_library_path(build_preset: str) -> Path:
    base_dir = (
        resolve_build_directory(
            REPO_ROOT,
            target="core",
            preset=build_preset,
            scope="shared",
        ).build_dir
        / "bin"
    )
    if sys.platform.startswith("win"):
        candidates = ["bills_core.dll", "libbills_core.dll"]
    elif sys.platform == "darwin":
        candidates = ["libbills_core.dylib", "bills_core.dylib"]
    else:
        candidates = ["libbills_core.so", "bills_core.so"]

    for candidate in candidates:
        path = base_dir / candidate
        if path.is_file():
            return path

    names = ", ".join(candidates)
    raise FileNotFoundError(
        f"Cannot find bills_core shared library under '{base_dir}'. Tried: {names}"
    )


def load_windows_dll_search_dirs() -> list[Path]:
    config = load_toolchain_config(WORKFLOW_TOML_PATH)
    resolved: list[Path] = []
    for item in config.verify.windows.dll_search_dirs:
        candidate = Path(item.strip())
        if not str(candidate).strip():
            continue
        if not candidate.is_absolute():
            candidate = (WORKFLOW_TOML_PATH.parent / candidate).resolve()
        resolved.append(candidate)
    return resolved
