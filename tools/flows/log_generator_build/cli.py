import argparse
from pathlib import Path

try:
    from ._bootstrap import bootstrap_repo_root
except ImportError:
    from _bootstrap import bootstrap_repo_root

from .artifact_pipeline import (
    promote_artifact_to_testdata,
    run_generate_to_artifact,
)
from .pipeline import run_build, run_target_only

REPO_ROOT = bootstrap_repo_root(__file__)

from tools.toolchain.services.build_layout import resolve_build_directory


def main(config: dict, project_dir, repo_root, argv=None) -> int:
    parser = argparse.ArgumentParser(description="LogGenerator Dist Tool")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    dist_parser = subparsers.add_parser("dist", help="Prepare the project into dist/cmake")
    dist_parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
        help="Preset to use.",
    )
    dist_parser.add_argument("extra", nargs="*", help="Extra arguments (e.g. clean)")

    generate_parser = subparsers.add_parser(
        "generate", help="Generate txt dataset to dist/tests/artifact"
    )
    generate_parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
        help="Preset to use.",
    )
    generate_parser.add_argument("--start-year", type=int, default=2024)
    generate_parser.add_argument("--end-year", type=int, default=2024)
    generate_parser.add_argument("--output-project", default="log_generator")
    generate_parser.add_argument("--skip-dist", action="store_true")

    promote_parser = subparsers.add_parser(
        "promote-testdata",
        help="Promote artifact dataset to testdata",
    )
    promote_parser.add_argument("--output-project", default="log_generator")
    promote_parser.add_argument(
        "--run-id",
        default="",
        help="Optional run id under dist/tests/artifact/<project>/runs/<run_id>.",
    )

    subparsers.add_parser("format", help="Run clang-format")
    subparsers.add_parser("tidy", help="Run clang-tidy fix")

    args = parser.parse_args(argv)

    if args.command == "dist":
        spec = resolve_build_directory(
            repo_root,
            target="bills-tracer-log-generator",
            preset=args.preset,
            scope="shared",
        )
        clean_flag = "clean" in args.extra
        run_build(
            spec.cmake_build_type,
            spec.build_dir,
            config,
            project_dir,
            clean_flag,
        )
        return 0

    if args.command == "generate":
        spec = resolve_build_directory(
            repo_root,
            target="bills-tracer-log-generator",
            preset=args.preset,
            scope="shared",
        )
        if not args.skip_dist:
            run_build(
                spec.cmake_build_type,
                spec.build_dir,
                config,
                project_dir,
                clean_flag=False,
            )
        return run_generate_to_artifact(
            repo_root=repo_root,
            project_dir=project_dir,
            build_dir=spec.build_dir,
            output_project=args.output_project,
            start_year=args.start_year,
            end_year=args.end_year,
        )

    if args.command == "promote-testdata":
        return promote_artifact_to_testdata(
            repo_root=repo_root,
            output_project=args.output_project,
            run_id=args.run_id.strip(),
        )

    if args.command == "format":
        spec = resolve_build_directory(
            repo_root,
            target="bills-tracer-log-generator",
            preset="debug",
            scope="shared",
        )
        run_target_only("Debug", spec.build_dir, "format", config, project_dir)
        return 0

    if args.command == "tidy":
        spec = resolve_build_directory(
            repo_root,
            target="bills-tracer-log-generator",
            preset="debug",
            scope="shared",
        )
        run_target_only("Debug", spec.build_dir, "tidy-fix", config, project_dir)
        return 0

    parser.print_help()
    return 1
