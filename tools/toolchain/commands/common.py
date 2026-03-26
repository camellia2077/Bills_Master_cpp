from __future__ import annotations

from ..core.context import Context
from ..services.dist.cli import run_cli_dist
from ..verify.cli import dispatch_workflow


def normalize_forwarded_args(forwarded: list[str]) -> list[str]:
    normalized = list(forwarded)
    while normalized and normalized[0] == "--":
        normalized = normalized[1:]
    return normalized


def run_verify_workflow(
    ctx: Context,
    workflow: str,
    forwarded: list[str] | None = None,
) -> tuple[list[str], int]:
    normalized = normalize_forwarded_args(forwarded or [])
    command = [ctx.python_executable, str(ctx.tools_root / "run.py"), "verify", workflow]
    if normalized:
        command.append("--")
        command.extend(normalized)
    return (
        command,
        dispatch_workflow(
            workflow,
            repo_root=ctx.repo_root,
            python_exe=ctx.python_executable,
            forwarded=normalized,
        ),
    )


def run_cli_dist_command(
    ctx: Context,
    *,
    preset: str = "debug",
    scope: str = "shared",
    forwarded: list[str] | None = None,
) -> tuple[list[str], int]:
    normalized = normalize_forwarded_args(forwarded or [])
    command = [
        ctx.python_executable,
        str(ctx.tools_root / "run.py"),
        "dist",
        "bills-tracer-cli",
        "--preset",
        preset,
        "--scope",
        scope,
    ]
    if normalized:
        command.extend(normalized)
    result = run_cli_dist(
        ctx.repo_root,
        preset=preset,
        scope=scope,
        extra_args=normalized,
    )
    return command, result.returncode


def resolve_keep_going(ctx: Context, keep_going: bool | None) -> bool:
    return ctx.config.tidy.keep_going if keep_going is None else keep_going
