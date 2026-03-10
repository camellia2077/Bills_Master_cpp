from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

from .config import ToolchainConfig, load_toolchain_config
from .process_runner import ProcessRunner


@dataclass(frozen=True)
class Context:
    repo_root: Path
    tools_root: Path
    temp_root: Path
    python_executable: str
    config_path: Path
    config: ToolchainConfig
    process_runner: ProcessRunner

    @classmethod
    def from_repo(cls) -> Context:
        tools_root = Path(__file__).resolve().parents[2]
        repo_root = tools_root.parent
        temp_root = repo_root / "temp"
        temp_root.mkdir(parents=True, exist_ok=True)
        config_path = tools_root / "toolchain" / "config" / "workflow.toml"
        config = load_toolchain_config(config_path)
        return cls(
            repo_root=repo_root,
            tools_root=tools_root,
            temp_root=temp_root,
            python_executable=sys.executable,
            config_path=config_path,
            config=config,
            process_runner=ProcessRunner(),
        )

    def flow_entry(self, relative_path: str) -> Path:
        return self.tools_root / "flows" / relative_path

    def verify_entry(self) -> Path:
        return self.tools_root / "verify" / "verify.py"
