from __future__ import annotations

from dataclasses import dataclass

from ..core.context import Context
from .common import forward_python_entry


@dataclass(frozen=True)
class BuildTargetSpec:
    entry_name: str
    default_args: list[str]


BUILD_TARGETS: dict[str, BuildTargetSpec] = {
    "bills": BuildTargetSpec(
        entry_name="build_bills_master.py",
        default_args=["build_fast"],
    ),
    "core": BuildTargetSpec(
        entry_name="build_bills_core.py",
        default_args=["build_fast"],
    ),
    "log-generator": BuildTargetSpec(
        entry_name="log_generator_flow.py",
        default_args=["build", "--mode", "Debug"],
    ),
}


def run(args, ctx: Context) -> int:
    target = str(args.target or ctx.config.build.default_target).strip()
    spec = BUILD_TARGETS[target]
    return forward_python_entry(
        ctx,
        ctx.flow_entry(spec.entry_name),
        forwarded=args.forwarded,
        default_args=spec.default_args,
    )
