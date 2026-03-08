from __future__ import annotations

import json
from pathlib import Path


def load_compile_units(compile_commands_path: Path) -> list[Path]:
    try:
        payload = json.loads(compile_commands_path.read_text(encoding="utf-8", errors="replace"))
    except (OSError, json.JSONDecodeError):
        return []

    if not isinstance(payload, list):
        return []

    files_by_key: dict[str, Path] = {}
    for item in payload:
        if not isinstance(item, dict):
            continue
        file_raw = item.get("file")
        if not isinstance(file_raw, str) or not file_raw.strip():
            continue
        file_path = Path(file_raw)
        if not file_path.is_absolute():
            directory_raw = item.get("directory")
            if isinstance(directory_raw, str) and directory_raw.strip():
                file_path = Path(directory_raw) / file_path
        files_by_key[path_key(file_path)] = file_path.resolve()
    return sorted(files_by_key.values(), key=lambda path: path_key(path))


def resolve_incremental_files(
    touched_files: list[Path],
    compile_units: list[Path],
    repo_root: Path,
    neighbor_scope: str,
) -> list[Path]:
    compile_by_key: dict[str, Path] = {}
    compile_by_dir: dict[str, set[Path]] = {}
    compile_by_module: dict[str, set[Path]] = {}
    for unit in compile_units:
        key = path_key(unit)
        compile_by_key[key] = unit
        dir_key = path_key(unit.parent)
        compile_by_dir.setdefault(dir_key, set()).add(unit)
        module = module_key(unit, repo_root)
        compile_by_module.setdefault(module, set()).add(unit)

    selected: set[Path] = set()
    unresolved: list[Path] = []
    touched_abs: list[Path] = []

    for touched in touched_files:
        absolute_touched = (
            touched.resolve() if touched.is_absolute() else (repo_root / touched).resolve()
        )
        touched_abs.append(absolute_touched)
        matched = match_compile_unit(absolute_touched, compile_by_key)
        if matched is not None:
            selected.add(matched)
        else:
            unresolved.append(absolute_touched)

    for unresolved_path in unresolved:
        same_dir_units = compile_by_dir.get(path_key(unresolved_path.parent), set())
        selected.update(same_dir_units)

    effective_scope = neighbor_scope if neighbor_scope in {"none", "dir", "module"} else "none"
    if effective_scope == "dir":
        seed_dirs = {path_key(path.parent) for path in touched_abs}
        seed_dirs.update(path_key(path.parent) for path in selected)
        for dir_key in seed_dirs:
            selected.update(compile_by_dir.get(dir_key, set()))
    elif effective_scope == "module":
        seed_modules = {module_key(path, repo_root) for path in touched_abs}
        seed_modules.update(module_key(path, repo_root) for path in selected)
        for module in seed_modules:
            selected.update(compile_by_module.get(module, set()))

    return sorted(selected, key=lambda path: path_key(path))


def match_compile_unit(
    touched_path: Path,
    compile_by_key: dict[str, Path],
) -> Path | None:
    direct_key = path_key(touched_path)
    if direct_key in compile_by_key:
        return compile_by_key[direct_key]
    normalized_variant = path_key(Path(str(touched_path).replace("/", "\\")))
    return compile_by_key.get(normalized_variant)


def module_key(file_path: Path, repo_root: Path) -> str:
    absolute_path = file_path.resolve()
    try:
        relative = absolute_path.relative_to(repo_root.resolve())
        parts = relative.parts
    except ValueError:
        parts = absolute_path.parts
    if len(parts) >= 3:
        return "/".join(part.lower() for part in parts[:3])
    if len(parts) >= 2:
        return "/".join(part.lower() for part in parts[:2])
    if parts:
        return parts[0].lower()
    return "."


def path_key(path: Path) -> str:
    normalized = str(path).replace("\\", "/")
    while "//" in normalized:
        normalized = normalized.replace("//", "/")
    return normalized.lower()
