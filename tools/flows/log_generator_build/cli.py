import argparse

from .artifact_pipeline import (
    promote_artifact_to_fixtures,
    run_generate_to_artifact,
)
from .pipeline import run_build, run_target_only


def main(config: dict, project_dir, repo_root, argv=None) -> int:
    parser = argparse.ArgumentParser(description="LogGenerator Build Tool")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    build_parser = subparsers.add_parser("build", help="Build the project")
    build_parser.add_argument(
        "--mode", choices=["Release", "Debug"], default="Release", help="Build mode"
    )
    build_parser.add_argument(
        "extra", nargs="*", help="Extra arguments (e.g. clean)"
    )

    generate_parser = subparsers.add_parser(
        "generate", help="Generate txt dataset to tests/output/artifact"
    )
    generate_parser.add_argument(
        "--mode", choices=["Release", "Debug"], default="Debug", help="Build mode"
    )
    generate_parser.add_argument("--start-year", type=int, default=2024)
    generate_parser.add_argument("--end-year", type=int, default=2024)
    generate_parser.add_argument("--output-project", default="log_generator")
    generate_parser.add_argument("--skip-build", action="store_true")

    promote_parser = subparsers.add_parser(
        "promote-fixtures",
        help="Promote artifact dataset to tests/fixtures",
    )
    promote_parser.add_argument("--output-project", default="log_generator")
    promote_parser.add_argument(
        "--run-id",
        default="",
        help="Optional run id under tests/output/artifact/<project>/runs/<run_id>.",
    )

    subparsers.add_parser("format", help="Run clang-format")
    subparsers.add_parser("tidy", help="Run clang-tidy fix")

    args = parser.parse_args(argv)

    if args.command == "build":
        build_dir = "build" if args.mode == "Release" else "build_debug"
        clean_flag = "clean" in args.extra
        run_build(args.mode, build_dir, config, project_dir, clean_flag)
        return 0

    if args.command == "generate":
        build_dir = "build" if args.mode == "Release" else "build_debug"
        if not args.skip_build:
            run_build(args.mode, build_dir, config, project_dir, clean_flag=False)
        return run_generate_to_artifact(
            repo_root=repo_root,
            project_dir=project_dir,
            build_dir_name=build_dir,
            output_project=args.output_project,
            start_year=args.start_year,
            end_year=args.end_year,
        )

    if args.command == "promote-fixtures":
        return promote_artifact_to_fixtures(
            repo_root=repo_root,
            output_project=args.output_project,
            run_id=args.run_id.strip(),
        )

    if args.command == "format":
        run_target_only("Debug", "build_debug", "format", config, project_dir)
        return 0

    if args.command == "tidy":
        run_target_only("Debug", "build_debug", "tidy-fix", config, project_dir)
        return 0

    parser.print_help()
    return 1
