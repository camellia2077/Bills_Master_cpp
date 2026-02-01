import sys
import argparse
from .pipeline import run_build, run_target_only

def main(config):
    """
    Main entry point for the automation CLI.
    Expects 'config' dictionary to be passed in.
    """
    parser = argparse.ArgumentParser(description="BillReprocessor Build Tool")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Command: build
    build_parser = subparsers.add_parser("build", help="Build the project")
    build_parser.add_argument("--mode", choices=["Release", "Debug"], default="Release", help="Build mode")
    build_parser.add_argument("extra", nargs="*", help="Extra arguments (e.g. clean)")

    # Command: format
    subparsers.add_parser("format", help="Run clang-format")

    # Command: tidy
    subparsers.add_parser("tidy", help="Run clang-tidy fix")

    args = parser.parse_args()

    if args.command == "build":
        build_dir = "build" if args.mode == "Release" else "build_debug"
        run_build(args.mode, build_dir, config)
    
    elif args.command == "format":
        # Use Debug build dir for tools
        run_target_only("Debug", "build_debug", "format", config)
        
    elif args.command == "tidy":
        # Use Debug build dir for tools
        run_target_only("Debug", "build_debug", "tidy-fix", config)
        
    else:
        parser.print_help()
