from __future__ import annotations

from dataclasses import dataclass

from ..core.context import Context
from ..services.build_layout import assert_no_legacy_flags
from .common import normalize_forwarded_args


@dataclass(frozen=True)
class BuildTargetSpec:
    entry_name: str
    fixed_args: list[str]
    supports_isolated: bool
    supports_tidy: bool


BUILD_TARGETS: dict[str, BuildTargetSpec] = {
    "bills-tracer-android": BuildTargetSpec(
        entry_name="build_bills_tracer_android.py",
        fixed_args=[],
        supports_isolated=False,
        supports_tidy=False,
    ),
    "bills-tracer-cli": BuildTargetSpec(
        entry_name="build_bills_tracer_cli.py",
        fixed_args=[],
        supports_isolated=True,
        supports_tidy=True,
    ),
    "bills-tracer-core": BuildTargetSpec(
        entry_name="build_bills_tracer_core.py",
        fixed_args=[],
        supports_isolated=False,
        supports_tidy=False,
    ),
    "bills-tracer-log-generator": BuildTargetSpec(
        entry_name="bills_tracer_log_generator_flow.py",
        fixed_args=["dist"],
        supports_isolated=False,
        supports_tidy=False,
    ),
}


def run(args, ctx: Context) -> int:
    target = str(args.target).strip()
    spec = BUILD_TARGETS[target]
    preset = str(args.preset).strip().lower()
    scope = str(args.scope).strip().lower()
    forwarded = normalize_forwarded_args(list(args.forwarded))
    assert_no_legacy_flags(forwarded, source="tools/run.py dist")

    if scope == "isolated" and not spec.supports_isolated:
        print(f"[ERROR] Target '{target}' only supports --scope shared.")
        return 2
    if preset == "tidy" and not spec.supports_tidy:
        print(f"[ERROR] Target '{target}' does not support --preset tidy.")
        return 2

    command = [
        ctx.python_executable,
        str(ctx.flow_entry(spec.entry_name)),
        *spec.fixed_args,
        "--preset",
        preset,
    ]
    if spec.supports_isolated:
        command.extend(["--scope", scope])
    if forwarded:
        command.extend(forwarded)

    result = ctx.process_runner.run(command, cwd=ctx.repo_root)
    return result.returncode
