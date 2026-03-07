from __future__ import annotations

from pathlib import Path

from ..core.context import Context


def normalize_forwarded_args(forwarded: list[str]) -> list[str]:
    if forwarded and forwarded[0] == "--":
        return forwarded[1:]
    return list(forwarded)


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


def run_python_command(
    ctx: Context,
    command: list[str],
    *,
    cwd: Path | None = None,
) -> int:
    result = ctx.process_runner.run(command, cwd=ctx.repo_root if cwd is None else cwd)
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


def report_unimplemented(command_name: str) -> int:
    print(
        f"[TODO] `{command_name}` is reserved by the unified toolchain skeleton "
        "and will be implemented in a later phase."
    )
    return 2
