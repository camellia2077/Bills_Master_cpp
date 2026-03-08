from __future__ import annotations

from pathlib import Path

from .models import StepSpec


def load_toml(path: Path) -> dict:
    try:
        import tomllib

        with path.open("rb") as handle:
            return tomllib.load(handle)
    except ModuleNotFoundError:
        import toml  # type: ignore

        return toml.loads(path.read_text(encoding="utf-8"))


def resolve_template(text: str, repo_root: Path, python_exe: str) -> str:
    return text.replace("{repo_root}", repo_root.as_posix()).replace("{python}", python_exe)


def build_step_specs(
    raw_steps: list[dict],
    repo_root: Path,
    python_exe: str,
    default_timeout_seconds: int,
) -> list[StepSpec]:
    steps: list[StepSpec] = []
    for index, raw in enumerate(raw_steps):
        step_id = str(raw.get("id", "")).strip()
        if not step_id:
            raise ValueError(f"steps[{index}] missing id")
        command = raw.get("command")
        if not isinstance(command, list) or not command:
            raise ValueError(f"steps[{index}] command must be non-empty list")
        command_args: list[str] = []
        for item in command:
            if not isinstance(item, str):
                raise ValueError(f"steps[{index}] command args must be string")
            command_args.append(resolve_template(item, repo_root, python_exe))

        raw_cwd = str(raw.get("cwd", "{repo_root}"))
        cwd = resolve_template(raw_cwd, repo_root, python_exe)
        depends_on_raw = raw.get("depends_on", [])
        if not isinstance(depends_on_raw, list):
            raise ValueError(f"steps[{index}] depends_on must be list")
        depends_on = [str(item) for item in depends_on_raw]
        timeout_seconds = int(raw.get("timeout_seconds", default_timeout_seconds))
        retries = int(raw.get("retries", 0))

        env_raw = raw.get("env", {})
        if not isinstance(env_raw, dict):
            raise ValueError(f"steps[{index}] env must be table")
        step_env = {
            str(k): resolve_template(str(v), repo_root, python_exe) for k, v in env_raw.items()
        }

        artifacts_raw = raw.get("artifacts", [])
        if not isinstance(artifacts_raw, list):
            raise ValueError(f"steps[{index}] artifacts must be list")
        artifacts = [str(item) for item in artifacts_raw]

        steps.append(
            StepSpec(
                step_id=step_id,
                name=str(raw.get("name", step_id)),
                command=command_args,
                cwd=cwd,
                depends_on=depends_on,
                timeout_seconds=timeout_seconds,
                retries=retries,
                env=step_env,
                artifacts=artifacts,
            )
        )
    return steps
