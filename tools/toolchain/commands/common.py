from __future__ import annotations

from pathlib import Path

from ..core.context import Context


def normalize_forwarded_args(forwarded: list[str]) -> list[str]:
    normalized = list(forwarded)
    while normalized and normalized[0] == "--":
        normalized = normalized[1:]
    return normalized


def forward_python_entry(
    ctx: Context,
    entry: Path,
    *,
    forwarded: list[str],
    default_args: list[str] | None = None,
) -> int:
    normalized = normalize_forwarded_args(forwarded)
    command_args = normalized if normalized else list(default_args or [])
    command = [ctx.python_executable, str(entry), *command_args]
    result = ctx.process_runner.run(command, cwd=ctx.repo_root)
    return result.returncode


def run_verify_workflow(
    ctx: Context,
    workflow: str,
    forwarded: list[str] | None = None,
) -> tuple[list[str], int]:
    normalized = normalize_forwarded_args(forwarded or [])
    command = [ctx.python_executable, str(ctx.verify_entry()), workflow]
    if normalized:
        command.append("--")
        command.extend(normalized)
    result = ctx.process_runner.run(command, cwd=ctx.repo_root)
    return command, result.returncode


def resolve_keep_going(ctx: Context, keep_going: bool | None) -> bool:
    return ctx.config.tidy.keep_going if keep_going is None else keep_going
