from __future__ import annotations

import argparse

from ..commands.build import run as run_build
from ..commands.clean import run as run_clean
from ..commands.format import run as run_format
from ..commands.tidy import run as run_tidy
from ..commands.tidy_batch import run as run_tidy_batch
from ..commands.tidy_close import run as run_tidy_close
from ..commands.tidy_fix import run as run_tidy_fix
from ..commands.tidy_list import run as run_tidy_list
from ..commands.tidy_loop import run as run_tidy_loop
from ..commands.tidy_next import run as run_tidy_next
from ..commands.tidy_refresh import run as run_tidy_refresh
from ..commands.tidy_show import run as run_tidy_show
from ..commands.tidy_split import run as run_tidy_split
from ..commands.verify import run as run_verify


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Unified Python toolchain entry for bills_tracer."
    )
    subparsers = parser.add_subparsers(dest="command")

    build_parser = subparsers.add_parser(
        "build",
        help="Forward to existing build helpers under tools/build.",
        description=(
            "Build one of the supported targets. "
            "Extra arguments are forwarded to the underlying build entry."
        ),
    )
    build_parser.add_argument(
        "target",
        nargs="?",
        choices=["bills", "core", "log-generator"],
        default=None,
        help="Build target to use (default from tools/toolchain/config/workflow.toml).",
    )
    build_parser.add_argument(
        "forwarded",
        nargs=argparse.REMAINDER,
        help=(
            "Arguments forwarded to the underlying build entry. "
            "Example: `python tools/run.py build core build_fast --compiler clang`."
        ),
    )
    build_parser.set_defaults(handler=run_build)

    verify_parser = subparsers.add_parser(
        "verify",
        help="Forward to tools/verify/verify.py.",
        description=(
            "Run the existing unified verify entry. "
            "Extra arguments are forwarded unchanged."
        ),
    )
    verify_parser.add_argument(
        "forwarded",
        nargs=argparse.REMAINDER,
        help=(
            "Arguments forwarded to tools/verify/verify.py. "
            "Use `-- --help` to inspect the downstream CLI help."
        ),
    )
    verify_parser.set_defaults(handler=run_verify)

    format_parser = subparsers.add_parser(
        "format",
        help="Run clang-format.",
        description="Run clang-format over repository C/C++ sources.",
    )
    format_parser.add_argument(
        "--check",
        action="store_true",
        help="Run clang-format in check mode instead of applying edits.",
    )
    format_parser.add_argument(
        "paths",
        nargs="*",
        help="Optional files or directories to limit the format scope.",
    )
    format_parser.set_defaults(handler=run_format)

    tidy_parser = subparsers.add_parser(
        "tidy",
        help="Run clang-tidy and capture raw logs.",
        description="Run clang-tidy through the existing bills build helper and capture raw logs for later splitting.",
    )
    tidy_parser.add_argument(
        "--build-dir",
        default="build_tidy",
        help="Build directory name under apps/bills_cli (default: build_tidy).",
    )
    tidy_parser.add_argument(
        "forwarded",
        nargs=argparse.REMAINDER,
        help="Extra args forwarded to the underlying tidy build.",
    )
    tidy_parser.set_defaults(handler=run_tidy)

    tidy_split_parser = subparsers.add_parser(
        "tidy-split",
        help="Split the latest tidy log into task and batch files.",
        description="Parse the latest captured clang-tidy log and emit task_*.log plus batch folders.",
    )
    tidy_split_parser.add_argument("--max-lines", type=int, default=None)
    tidy_split_parser.add_argument("--max-diags", type=int, default=None)
    tidy_split_parser.add_argument("--batch-size", type=int, default=None)
    tidy_split_parser.set_defaults(handler=run_tidy_split)

    tidy_batch_parser = subparsers.add_parser(
        "tidy-batch",
        help="Run verify/clean/refresh/finalize for one batch.",
        description="Process a single tidy batch through verify, clean, refresh, and finalize stages.",
    )
    tidy_batch_parser.add_argument("--batch-id", required=True)
    tidy_batch_parser.add_argument("--preset", choices=["sop"], default=None)
    tidy_batch_parser.add_argument("--strict-clean", action="store_true")
    tidy_batch_parser.add_argument("--run-verify", action="store_true")
    tidy_batch_parser.add_argument("--full-every", type=int, default=3)
    tidy_batch_parser.add_argument("--concise", action="store_true")
    tidy_batch_parser.add_argument("--timeout-seconds", type=int, default=None)
    tidy_batch_keep_going_group = tidy_batch_parser.add_mutually_exclusive_group()
    tidy_batch_keep_going_group.add_argument(
        "--keep-going", dest="keep_going", action="store_true", default=None
    )
    tidy_batch_keep_going_group.add_argument(
        "--no-keep-going", dest="keep_going", action="store_false"
    )
    tidy_batch_parser.set_defaults(handler=run_tidy_batch)

    tidy_refresh_parser = subparsers.add_parser(
        "tidy-refresh",
        help="Refresh the tidy queue after one batch.",
        description="Run incremental or full tidy refresh after tasks have been cleaned.",
    )
    tidy_refresh_parser.add_argument("--batch-id", default="")
    tidy_refresh_parser.add_argument("--full-every", type=int, default=3)
    tidy_refresh_parser.add_argument("--force-full", action="store_true")
    tidy_refresh_parser.add_argument("--final-full", action="store_true")
    tidy_refresh_parser.add_argument("--dry-run", action="store_true")
    tidy_refresh_parser.add_argument(
        "--neighbor-scope",
        choices=["none", "dir", "module"],
        default=None,
        help="Expand incremental compile units by directory or module after same-dir fallback.",
    )
    tidy_refresh_keep_going_group = tidy_refresh_parser.add_mutually_exclusive_group()
    tidy_refresh_keep_going_group.add_argument(
        "--keep-going", dest="keep_going", action="store_true", default=None
    )
    tidy_refresh_keep_going_group.add_argument(
        "--no-keep-going", dest="keep_going", action="store_false"
    )
    tidy_refresh_parser.set_defaults(handler=run_tidy_refresh)

    tidy_close_parser = subparsers.add_parser(
        "tidy-close",
        help="Run the final tidy close flow.",
        description="Run final-full tidy refresh, optional verify, and ensure no pending tasks remain.",
    )
    tidy_close_parser.add_argument("--tidy-only", action="store_true")
    tidy_close_parser.add_argument("--concise", action="store_true")
    tidy_close_keep_going_group = tidy_close_parser.add_mutually_exclusive_group()
    tidy_close_keep_going_group.add_argument(
        "--keep-going", dest="keep_going", action="store_true", default=None
    )
    tidy_close_keep_going_group.add_argument(
        "--no-keep-going", dest="keep_going", action="store_false"
    )
    tidy_close_parser.set_defaults(handler=run_tidy_close)

    tidy_loop_parser = subparsers.add_parser(
        "tidy-loop",
        help="Process auto-fixable tidy tasks in a loop.",
        description="Walk pending tidy tasks, clean auto-fixable ones, and run verify on cadence.",
    )
    tidy_loop_target_group = tidy_loop_parser.add_mutually_exclusive_group()
    tidy_loop_target_group.add_argument("--n", type=int, default=None)
    tidy_loop_target_group.add_argument("--all", action="store_true")
    tidy_loop_parser.add_argument("--test-every", type=int, default=1)
    tidy_loop_parser.add_argument("--concise", action="store_true")
    tidy_loop_parser.set_defaults(handler=run_tidy_loop)

    clean_parser = subparsers.add_parser(
        "clean",
        help="Archive completed tidy tasks.",
        description="Move verified task logs from temp/tidy/tasks into temp/tidy/tasks_done.",
    )
    clean_parser.add_argument("--batch-id", default=None)
    clean_parser.add_argument("--strict", action="store_true")
    clean_parser.add_argument("--cluster-by-file", action="store_true")
    clean_parser.add_argument("task_ids", nargs="*")
    clean_parser.set_defaults(handler=run_clean)

    tidy_list_parser = subparsers.add_parser(
        "tidy-list",
        help="List pending tidy batches.",
        description="Show pending tidy batches and their current status.",
    )
    tidy_list_parser.set_defaults(handler=run_tidy_list)

    tidy_next_parser = subparsers.add_parser(
        "tidy-next",
        help="Show the next open tidy batch.",
        description="Print the next pending tidy batch and the recommended command.",
    )
    tidy_next_parser.set_defaults(handler=run_tidy_next)

    tidy_show_parser = subparsers.add_parser(
        "tidy-show",
        help="Show task details for one tidy batch.",
        description="Print task/file/check details for a single pending tidy batch.",
    )
    tidy_show_parser.add_argument("--batch-id", required=True)
    tidy_show_parser.set_defaults(handler=run_tidy_show)

    tidy_fix_parser = subparsers.add_parser(
        "tidy-fix",
        help="Run clang-tidy -fix on one batch or explicit paths.",
        description="Apply clang-tidy -fix to the files in a batch or to explicit source paths.",
    )
    tidy_fix_parser.add_argument("--batch-id", default=None)
    tidy_fix_parser.add_argument("paths", nargs="*")
    tidy_fix_parser.set_defaults(handler=run_tidy_fix)

    return parser
