from __future__ import annotations

from pathlib import Path

from ..core.context import Context
from ..services.clang_tidy_runner import run_clang_tidy
from ..services.file_discovery import discover_source_files
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks


def execute_tidy_fix(
    ctx: Context,
    *,
    batch_id: str | None,
    explicit_paths: list[str],
    output_log: Path | None = None,
) -> int:
    paths = resolve_tidy_paths(ctx)
    if not paths.compile_commands_path.exists():
        print(
            "[ERROR] Missing compile_commands.json. "
            "Run `python tools/run.py tidy` first."
        )
        return 1

    target_files: list[Path] = []
    seen: set[str] = set()
    normalized_batch = (batch_id or "").strip()
    if normalized_batch:
        for task in load_batch_tasks(paths.tasks_manifest, normalized_batch):
            source_path = _resolve_source_path(ctx, str(task.get("source_file", "")))
            key = _path_key(source_path)
            if key in seen:
                continue
            seen.add(key)
            target_files.append(source_path)

    for source_path in discover_source_files(ctx.repo_root, explicit_paths=explicit_paths):
        key = _path_key(source_path)
        if key in seen:
            continue
        seen.add(key)
        target_files.append(source_path)

    if not target_files:
        print("[ERROR] tidy-fix could not resolve any target source files.")
        return 1

    log_path = output_log or (paths.refresh_dir / f"{normalized_batch or 'manual'}_fix.log")
    returncode = run_clang_tidy(
        ctx,
        compile_commands_dir=paths.compile_commands_path.parent,
        files=target_files,
        output_log=log_path,
        fix=True,
    )
    print(f"--- tidy-fix: log -> {log_path}")
    return returncode


def run(args, ctx: Context) -> int:
    return execute_tidy_fix(
        ctx,
        batch_id=args.batch_id,
        explicit_paths=list(args.paths),
    )


def _resolve_source_path(ctx: Context, source_file: str) -> Path:
    candidate = Path(source_file)
    if candidate.is_absolute():
        return candidate
    return (ctx.repo_root / candidate).resolve()


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()
