from __future__ import annotations

import hashlib
from dataclasses import dataclass
from pathlib import Path

from ..core.context import Context
from ..core.path_display import display_path
from ..services.clang_tidy_runner import run_clang_tidy
from ..services.file_discovery import discover_source_files
from ..services.tidy_paths import resolve_tidy_paths
from ..services.tidy_queue import load_batch_tasks


@dataclass(frozen=True)
class TidyFixResult:
    returncode: int
    log_path: Path
    target_files: list[Path]
    changed_files: list[Path]
    checks_filter: list[str]


def run_tidy_fix_pass(
    ctx: Context,
    *,
    batch_id: str | None,
    explicit_paths: list[str],
    output_log: Path | None = None,
    checks_filter: list[str] | None = None,
) -> TidyFixResult:
    paths = resolve_tidy_paths(ctx)
    if not paths.compile_commands_path.exists():
        print(
            "[ERROR] Missing compile_commands.json. "
            "Run `python tools/run.py tidy` first."
        )
        log_path = output_log or (paths.refresh_dir / f"{(batch_id or 'manual').strip() or 'manual'}_fix.log")
        return TidyFixResult(
            returncode=1,
            log_path=log_path,
            target_files=[],
            changed_files=[],
            checks_filter=list(checks_filter or []),
        )

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

    if explicit_paths:
        for source_path in discover_source_files(
            ctx.repo_root,
            scope_config=ctx.config.scope,
            explicit_paths=explicit_paths,
        ):
            key = _path_key(source_path)
            if key in seen:
                continue
            seen.add(key)
            target_files.append(source_path)

    if not target_files:
        print("[ERROR] tidy-fix could not resolve any target source files.")
        log_path = output_log or (paths.refresh_dir / f"{normalized_batch or 'manual'}_fix.log")
        return TidyFixResult(
            returncode=1,
            log_path=log_path,
            target_files=[],
            changed_files=[],
            checks_filter=list(checks_filter or []),
        )

    log_path = output_log or (paths.refresh_dir / f"{normalized_batch or 'manual'}_fix.log")
    before_hashes = {
        _path_key(file_path): _hash_file(file_path) for file_path in target_files if file_path.exists()
    }
    returncode = run_clang_tidy(
        ctx,
        compile_commands_dir=paths.compile_commands_path.parent,
        files=target_files,
        output_log=log_path,
        fix=True,
        checks_filter=list(checks_filter or []),
    )
    changed_files: list[Path] = []
    for file_path in target_files:
        if not file_path.exists():
            continue
        key = _path_key(file_path)
        before_hash = before_hashes.get(key)
        after_hash = _hash_file(file_path)
        if before_hash != after_hash:
            changed_files.append(file_path)
    print(f"--- tidy-fix: log -> {display_path(log_path, resolve=True)}")
    return TidyFixResult(
        returncode=returncode,
        log_path=log_path,
        target_files=target_files,
        changed_files=changed_files,
        checks_filter=list(checks_filter or []),
    )


def execute_tidy_fix(
    ctx: Context,
    *,
    batch_id: str | None,
    explicit_paths: list[str],
    output_log: Path | None = None,
    checks_filter: list[str] | None = None,
) -> int:
    result = run_tidy_fix_pass(
        ctx,
        batch_id=batch_id,
        explicit_paths=explicit_paths,
        output_log=output_log,
        checks_filter=checks_filter,
    )
    return result.returncode


def run(args, ctx: Context) -> int:
    return execute_tidy_fix(
        ctx,
        batch_id=args.batch_id,
        explicit_paths=list(args.paths),
        checks_filter=list(getattr(args, "checks", []) or []),
    )


def _resolve_source_path(ctx: Context, source_file: str) -> Path:
    candidate = Path(source_file)
    if candidate.is_absolute():
        return candidate
    return (ctx.repo_root / candidate).resolve()


def _path_key(path: Path) -> str:
    return str(path).replace("\\", "/").lower()


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(8192), b""):
            digest.update(chunk)
    return digest.hexdigest()
