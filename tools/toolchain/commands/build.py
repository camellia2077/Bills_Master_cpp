from __future__ import annotations

from ..core.context import Context
from ..services.build_layout import assert_no_legacy_flags
from ..services.dist import run_android_dist, run_cli_dist, run_core_dist, run_log_generator_dist
from .common import normalize_forwarded_args


def run(args, ctx: Context) -> int:
    target = str(args.target).strip()
    preset = str(args.preset).strip().lower()
    scope = str(args.scope).strip().lower()
    forwarded = normalize_forwarded_args(list(args.forwarded))
    assert_no_legacy_flags(forwarded, source="tools/run.py dist")
    if scope == "isolated" and target != "bills-tracer-cli":
        print(f"[ERROR] Target '{target}' only supports --scope shared.")
        return 2
    if preset == "tidy" and target not in {"bills-tracer-cli"}:
        print(f"[ERROR] Target '{target}' does not support --preset tidy.")
        return 2

    try:
        if target == "bills-tracer-cli":
            result = run_cli_dist(
                ctx.repo_root,
                preset=preset,
                scope=scope,
                extra_args=forwarded,
            )
            return result.returncode
        if target == "bills-tracer-core":
            return run_core_dist(
                ctx.repo_root,
                preset=preset,
                extra_args=forwarded,
            )
        if target == "bills-tracer-log-generator":
            return run_log_generator_dist(
                ctx.repo_root,
                preset=preset,
                extra_args=forwarded,
            )
        if target == "bills-tracer-android":
            android_args = ["--preset", preset, *forwarded]
            return run_android_dist(ctx.repo_root, android_args)
    except SystemExit as exc:
        return int(exc.code) if isinstance(exc.code, int) else 1

    print(f"[ERROR] Unsupported dist target '{target}'.")
    return 2
