from __future__ import annotations

import argparse

from ..commands.build import run as run_build
from ..commands.clean import run as run_clean
from ..commands.format import run as run_format
from ..commands.import_gate import run as run_import_gate
from ..commands.log_generator import run as run_log_generator
from ..commands.tidy import run as run_tidy
from ..commands.tidy_batch import run as run_tidy_batch
from ..commands.tidy_close import run as run_tidy_close
from ..commands.tidy_fix import run as run_tidy_fix
from ..commands.tidy_list import run as run_tidy_list
from ..commands.tidy_loop import run as run_tidy_loop
from ..commands.tidy_next import run as run_tidy_next
from ..commands.tidy_recheck import run as run_tidy_recheck
from ..commands.tidy_refresh import run as run_tidy_refresh
from ..commands.tidy_scope import run as run_tidy_scope
from ..commands.tidy_show import run as run_tidy_show
from ..commands.tidy_split import run as run_tidy_split
from ..commands.tidy_status import run as run_tidy_status
from ..commands.verify import run as run_verify


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Unified Python toolchain entry for bills_tracer.")
    subparsers = parser.add_subparsers(dest="command")

    dist_parser = subparsers.add_parser(
        "dist",
        help="Forward to the supported dist helpers under tools/flows.",
        description=(
            "Prepare one of the supported targets into dist/. "
            "Unknown extra arguments are forwarded to the underlying entry."
        ),
    )
    dist_parser.add_argument(
        "target",
        choices=[
            "bills-tracer-cli",
            "bills-tracer-core",
            "bills-tracer-log-generator",
            "bills-tracer-android",
        ],
        help="Target to emit into dist/.",
    )
    dist_parser.add_argument(
        "--preset",
        choices=["debug", "release", "tidy"],
        default="debug",
        help="Preset to use.",
    )
    dist_parser.add_argument(
        "--scope",
        choices=["shared", "isolated"],
        default="shared",
        help="Dist scope to use. Some targets only support shared.",
    )
    dist_parser.set_defaults(handler=run_build, forwarded=[])

    verify_parser = subparsers.add_parser(
        "verify",
        help="Run the unified verify workflows.",
        description=(
            "Run the unified verify workflows. Extra arguments are forwarded unchanged."
        ),
    )
    verify_parser.add_argument(
        "forwarded",
        nargs=argparse.REMAINDER,
        help=(
            "Arguments forwarded to the selected verify workflow. "
            "Use `-- --help` to inspect the downstream CLI help."
        ),
    )
    verify_parser.set_defaults(handler=run_verify)

    import_gate_parser = subparsers.add_parser(
        "import-gate",
        help="Validate Windows import tables for dist artifacts.",
        description="Inspect dist binaries and reject forbidden Windows runtime imports.",
    )
    import_gate_parser.add_argument(
        "target",
        choices=[
            "bills-tracer-cli",
            "bills-tracer-log-generator",
        ],
        help="Target to inspect under dist/cmake.",
    )
    import_gate_parser.add_argument(
        "--preset",
        choices=["debug", "release", "tidy"],
        default="debug",
        help="Preset to inspect.",
    )
    import_gate_parser.add_argument(
        "--scope",
        choices=["shared", "isolated"],
        default="shared",
        help="Dist scope to inspect.",
    )
    import_gate_parser.set_defaults(handler=run_import_gate)

    log_generator_parser = subparsers.add_parser(
        "log-generator",
        help="Run log_generator workflows.",
        description="Unified entry for log_generator dist, generation, and testdata promotion.",
    )
    log_generator_subparsers = log_generator_parser.add_subparsers(dest="log_generator_command")

    log_generator_dist_parser = log_generator_subparsers.add_parser(
        "dist",
        help="Prepare log_generator into dist/cmake.",
    )
    log_generator_dist_parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
    )
    log_generator_dist_parser.set_defaults(handler=run_log_generator)

    log_generator_generate_parser = log_generator_subparsers.add_parser(
        "generate",
        help="Generate artifact datasets via log_generator.",
    )
    log_generator_generate_parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
    )
    log_generator_generate_parser.add_argument("--start-year", type=int, default=2024)
    log_generator_generate_parser.add_argument("--end-year", type=int, default=2024)
    log_generator_generate_parser.add_argument("--output-project", default="log_generator")
    log_generator_generate_parser.add_argument("--skip-dist", action="store_true")
    log_generator_generate_parser.set_defaults(handler=run_log_generator)

    log_generator_promote_parser = log_generator_subparsers.add_parser(
        "promote-testdata",
        help="Promote generated artifact data into testdata/bills.",
    )
    log_generator_promote_parser.add_argument("--output-project", default="log_generator")
    log_generator_promote_parser.add_argument("--run-id", default="")
    log_generator_promote_parser.set_defaults(handler=run_log_generator)

    log_generator_format_parser = log_generator_subparsers.add_parser(
        "format",
        help="Run log_generator format target.",
    )
    log_generator_format_parser.set_defaults(handler=run_log_generator)

    log_generator_tidy_parser = log_generator_subparsers.add_parser(
        "tidy",
        help="Run log_generator tidy target.",
    )
    log_generator_tidy_parser.set_defaults(handler=run_log_generator)

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

    tidy_scope_parser = subparsers.add_parser(
        "tidy-scope",
        help="Show the resolved clang-format/clang-tidy scope.",
        description="Print the effective source roots, excludes, header filter, and file counts from workflow.toml.",
    )
    tidy_scope_parser.add_argument(
        "--include-optional",
        action="store_true",
        help="Include optional roots such as tests/generators/log_generator/src.",
    )
    tidy_scope_parser.add_argument(
        "--show-files",
        action="store_true",
        help="Print every resolved file in the current scope.",
    )
    tidy_scope_parser.set_defaults(handler=run_tidy_scope)

    tidy_parser = subparsers.add_parser(
        "tidy",
        help="Run clang-tidy and capture raw logs.",
        description="Run clang-tidy through the existing bills_tracer_cli dist helper and capture raw logs for later splitting.",
    )
    tidy_parser.add_argument(
        "--jobs",
        type=int,
        default=None,
        help="Parallel Ninja jobs for full tidy dist preparation, e.g. 16.",
    )
    tidy_keep_going_group = tidy_parser.add_mutually_exclusive_group()
    tidy_keep_going_group.add_argument(
        "--keep-going", dest="keep_going", action="store_true", default=None
    )
    tidy_keep_going_group.add_argument("--no-keep-going", dest="keep_going", action="store_false")
    tidy_parser.set_defaults(handler=run_tidy, forwarded=[])

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

    tidy_status_parser = subparsers.add_parser(
        "tidy-status",
        help="Show global or per-batch tidy SOP state.",
        description=(
            "Read the canonical tidy state and print queue progress, "
            "phase history, remaining diagnostics, and numbering context."
        ),
    )
    tidy_status_parser.add_argument("--batch-id", default=None)
    tidy_status_parser.set_defaults(handler=run_tidy_status)

    tidy_fix_parser = subparsers.add_parser(
        "tidy-fix",
        help="Run clang-tidy -fix on one batch or explicit paths.",
        description="Apply clang-tidy -fix to the files in a batch or to explicit source paths.",
    )
    tidy_fix_parser.add_argument("--batch-id", default=None)
    tidy_fix_parser.add_argument(
        "--checks",
        nargs="*",
        default=None,
        help="Optional clang-tidy check filter used for fix-only passes.",
    )
    tidy_fix_parser.add_argument("paths", nargs="*")
    tidy_fix_parser.set_defaults(handler=run_tidy_fix)

    tidy_recheck_parser = subparsers.add_parser(
        "tidy-recheck",
        help="Run targeted clang-tidy recheck for one batch.",
        description=(
            "Run targeted clang-tidy on the batch source files, persist "
            "structured remaining diagnostics, and refresh batch status."
        ),
    )
    tidy_recheck_parser.add_argument("--batch-id", required=True)
    tidy_recheck_parser.set_defaults(handler=run_tidy_recheck)

    return parser
