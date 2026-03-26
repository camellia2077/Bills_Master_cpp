from __future__ import annotations

from ..core.context import Context
from ..services.log_generator.runner import run_log_generator_command


def run(args, ctx: Context) -> int:
    argv = [str(args.log_generator_command).strip()]
    if args.log_generator_command in {"dist", "generate"}:
        argv.extend(["--preset", str(args.preset).strip()])
    if args.log_generator_command == "generate":
        argv.extend(
            [
                "--start-year",
                str(int(args.start_year)),
                "--end-year",
                str(int(args.end_year)),
                "--output-project",
                str(args.output_project).strip(),
            ]
        )
        if args.skip_dist:
            argv.append("--skip-dist")
    elif args.log_generator_command == "promote-testdata":
        argv.extend(["--output-project", str(args.output_project).strip()])
        if str(args.run_id).strip():
            argv.extend(["--run-id", str(args.run_id).strip()])
    return run_log_generator_command(ctx.repo_root, argv)
