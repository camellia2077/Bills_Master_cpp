from __future__ import annotations

from ..core.context import Context
from ..core.path_display import display_path, display_path_from_repo
from ..services.file_discovery import (
    COMPILE_UNIT_EXTENSIONS,
    build_header_filter_regex,
    discover_source_files,
    resolve_excluded_segments,
    resolve_header_filter_roots,
    resolve_source_roots,
)


def run(args, ctx: Context) -> int:
    include_optional = bool(args.include_optional)
    scope_config = ctx.config.scope
    default_roots = resolve_source_roots(scope_config)
    optional_roots = list(scope_config.optional_roots)
    effective_roots = resolve_source_roots(
        scope_config,
        include_optional_roots=include_optional,
    )
    header_filter_roots = resolve_header_filter_roots(scope_config)
    header_filter_regex = build_header_filter_regex(header_filter_roots)
    excluded_segments = sorted(resolve_excluded_segments(scope_config))

    all_files = discover_source_files(
        ctx.repo_root,
        scope_config=scope_config,
        include_optional_roots=include_optional,
    )
    compile_units = discover_source_files(
        ctx.repo_root,
        scope_config=scope_config,
        include_optional_roots=include_optional,
        extensions=COMPILE_UNIT_EXTENSIONS,
    )
    header_count = len(all_files) - len(compile_units)

    print("tidy-scope")
    print(f"- repo_root: {display_path(ctx.repo_root, resolve=True)}")
    print(f"- config: {display_path(ctx.config_path, resolve=True)}")
    print(f"- include_optional: {str(include_optional).lower()}")
    print("- default_roots:")
    for root in default_roots:
        print(f"  - {display_path(root)}")
    print("- optional_roots:")
    if optional_roots:
        for root in optional_roots:
            print(f"  - {display_path(root)}")
    else:
        print("  - <none>")
    print("- effective_roots:")
    for root in effective_roots:
        print(f"  - {display_path(root)}")
    print("- exclude_segments:")
    for segment in excluded_segments:
        print(f"  - {segment}")
    print("- header_filter_roots:")
    for root in header_filter_roots:
        print(f"  - {display_path(root)}")
    print(f"- header_filter_regex: {header_filter_regex}")
    print(f"- discovered_files: {len(all_files)}")
    print(f"- compile_units: {len(compile_units)}")
    print(f"- headers: {header_count}")
    if args.show_files:
        print("- files:")
        for file_path in all_files:
            print(f"  - {display_path_from_repo(ctx.repo_root, file_path)}")
    return 0
