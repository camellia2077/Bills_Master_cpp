import argparse

from .pipeline import run_build, run_target_only


def main(config: dict, project_dir, argv=None) -> int:
    parser = argparse.ArgumentParser(description="LogGenerator Build Tool")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    build_parser = subparsers.add_parser("build", help="Build the project")
    build_parser.add_argument(
        "--mode", choices=["Release", "Debug"], default="Release", help="Build mode"
    )
    build_parser.add_argument(
        "extra", nargs="*", help="Extra arguments (e.g. clean)"
    )

    subparsers.add_parser("format", help="Run clang-format")
    subparsers.add_parser("tidy", help="Run clang-tidy fix")

    args = parser.parse_args(argv)

    if args.command == "build":
        build_dir = "build" if args.mode == "Release" else "build_debug"
        clean_flag = "clean" in args.extra
        run_build(args.mode, build_dir, config, project_dir, clean_flag)
        return 0

    if args.command == "format":
        run_target_only("Debug", "build_debug", "format", config, project_dir)
        return 0

    if args.command == "tidy":
        run_target_only("Debug", "build_debug", "tidy-fix", config, project_dir)
        return 0

    parser.print_help()
    return 1
