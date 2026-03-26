from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

from .models import IncludeRecord
from .policy import classify_entry, reason_for_entry
from .scan import BOUNDARY_LAYER_ROOTS, build_observed_quoted_includes

SCHEMA_VERSION = 1
POLICY_TEXT = (
    "Boundary layer allows quoted includes, but each entry must define "
    "owner/reason/window/tier (replaceable vs long-term)."
)


def normalize_config_path(repo_root: Path, config_arg: str) -> Path:
    config_path = Path(config_arg)
    if config_path.is_absolute():
        return config_path
    return (repo_root / config_path).resolve()


def _config_uses_directory(config_path: Path) -> bool:
    if config_path.exists():
        return config_path.is_dir()
    return config_path.suffix.lower() != ".json"


def _iter_config_files(config_path: Path) -> tuple[list[Path], list[str]]:
    if not config_path.exists():
        return [], [f"Allowlist config does not exist: {config_path}"]

    if config_path.is_dir():
        config_files = sorted(
            path for path in config_path.iterdir() if path.is_file() and path.suffix.lower() == ".json"
        )
        if not config_files:
            return [], [f"Allowlist config directory is empty: {config_path}"]
        return config_files, []

    return [config_path], []


def _load_json_file(config_file: Path) -> tuple[dict[str, object] | None, list[str]]:
    try:
        content = json.loads(config_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return None, [f"Invalid allowlist config JSON: {config_file} ({exc})"]

    if not isinstance(content, dict):
        return None, [f"Invalid allowlist config JSON: {config_file} (top-level must be object)"]
    return content, []


def _scope_name_for_file_path(file_path: str) -> str:
    normalized_path = file_path.replace("\\", "/")
    for scope_name, scope_root in BOUNDARY_LAYER_ROOTS.items():
        normalized_root = scope_root.replace("\\", "/").rstrip("/") + "/"
        if normalized_path.startswith(normalized_root):
            return scope_name
    raise ValueError(f"Unable to classify boundary allowlist entry: {file_path}")


def _build_payload(
    ordered_allowlist: dict[str, list[dict[str, str]]],
    generated_at: str,
    scope_name: str | None = None,
) -> dict[str, object]:
    payload: dict[str, object] = {
        "schema_version": SCHEMA_VERSION,
        "generated_at": generated_at,
        "policy": POLICY_TEXT,
        "boundary_layer_roots": BOUNDARY_LAYER_ROOTS,
        "allowed_quoted_includes": ordered_allowlist,
    }
    if scope_name is not None:
        payload["scope"] = scope_name
    return payload


def existing_metadata(config_path: Path) -> dict[tuple[str, str], dict[str, str]]:
    config_files, errors = _iter_config_files(config_path)
    if errors:
        return {}

    result: dict[tuple[str, str], dict[str, str]] = {}
    for config_file in config_files:
        content, load_errors = _load_json_file(config_file)
        if load_errors or content is None:
            continue

        raw_allowlist = content.get("allowed_quoted_includes", {})
        if not isinstance(raw_allowlist, dict):
            continue

        for file_path, entries in raw_allowlist.items():
            if not isinstance(file_path, str) or not isinstance(entries, list):
                continue
            normalized_path = file_path.replace("\\", "/")
            for entry in entries:
                if not isinstance(entry, dict):
                    continue
                header = entry.get("header")
                if not isinstance(header, str) or not header:
                    continue
                result[(normalized_path, header)] = {
                    "owner": str(entry.get("owner", "")).strip(),
                    "reason": str(entry.get("reason", "")).strip(),
                    "window": str(entry.get("window", "")).strip(),
                    "tier": str(entry.get("tier", "")).strip(),
                }
    return result


def _build_ordered_allowlist(
    include_records: list[IncludeRecord],
    baseline_owner: str,
    preserved: dict[tuple[str, str], dict[str, str]],
) -> dict[str, list[dict[str, str]]]:
    observed = build_observed_quoted_includes(include_records)
    ordered_allowlist: dict[str, list[dict[str, str]]] = {}
    for file_path in sorted(observed.keys()):
        headers = sorted(observed[file_path])
        items: list[dict[str, str]] = []
        for header in headers:
            key = (file_path, header)
            current_meta = preserved.get(key, {})
            tier, window = classify_entry(file_path, header)
            tier = current_meta.get("tier") or tier
            window = current_meta.get("window") or window
            reason = current_meta.get("reason") or reason_for_entry(
                file_path=file_path,
                header=header,
                tier=tier,
            )
            owner = current_meta.get("owner") or baseline_owner

            items.append(
                {
                    "header": header,
                    "owner": owner,
                    "reason": reason,
                    "window": window,
                    "tier": tier,
                }
            )
        ordered_allowlist[file_path] = items
    return ordered_allowlist


def _write_single_file_baseline(
    config_path: Path,
    ordered_allowlist: dict[str, list[dict[str, str]]],
    generated_at: str,
) -> None:
    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        json.dumps(_build_payload(ordered_allowlist, generated_at), indent=2, ensure_ascii=False)
        + "\n",
        encoding="utf-8",
    )


def _write_sharded_baseline(
    config_path: Path,
    ordered_allowlist: dict[str, list[dict[str, str]]],
    generated_at: str,
) -> None:
    config_path.mkdir(parents=True, exist_ok=True)

    shards: dict[str, dict[str, list[dict[str, str]]]] = {}
    for file_path, entries in ordered_allowlist.items():
        scope_name = _scope_name_for_file_path(file_path)
        shards.setdefault(scope_name, {})[file_path] = entries

    written_files: set[str] = set()
    for scope_name, scope_allowlist in sorted(shards.items()):
        shard_path = config_path / f"{scope_name}.json"
        written_files.add(shard_path.name)
        shard_path.write_text(
            json.dumps(
                _build_payload(scope_allowlist, generated_at, scope_name=scope_name),
                indent=2,
                ensure_ascii=False,
            )
            + "\n",
            encoding="utf-8",
        )

    for stale_file in sorted(config_path.iterdir()):
        if not stale_file.is_file() or stale_file.suffix.lower() != ".json":
            continue
        if stale_file.name in written_files:
            continue
        stale_file.unlink()


def write_baseline(
    config_path: Path,
    include_records: list[IncludeRecord],
    baseline_owner: str,
) -> None:
    preserved = existing_metadata(config_path)
    ordered_allowlist = _build_ordered_allowlist(include_records, baseline_owner, preserved)
    generated_at = datetime.now().isoformat(timespec="seconds")

    if _config_uses_directory(config_path):
        _write_sharded_baseline(config_path, ordered_allowlist, generated_at)
    else:
        _write_single_file_baseline(config_path, ordered_allowlist, generated_at)

    total_entries = sum(len(items) for items in ordered_allowlist.values())
    print(
        "[OK] boundary include allowlist baseline generated: "
        f"{config_path} (entries={total_entries})"
    )


def load_allowlist(
    config_path: Path,
) -> tuple[set[tuple[str, str]], list[str], dict[tuple[str, str], str]]:
    config_files, errors = _iter_config_files(config_path)
    if errors:
        return set(), errors, {}

    allowed: set[tuple[str, str]] = set()
    schema_errors: list[str] = []
    tier_map: dict[tuple[str, str], str] = {}
    valid_tiers = {"replaceable", "long-term"}

    for config_file in config_files:
        content, load_errors = _load_json_file(config_file)
        if load_errors:
            schema_errors.extend(load_errors)
            continue
        assert content is not None

        raw_allowlist = content.get("allowed_quoted_includes")
        if not isinstance(raw_allowlist, dict):
            schema_errors.append(
                f"allowlist schema error: '{config_file}' -> 'allowed_quoted_includes' must be object"
            )
            continue

        for file_path, entries in raw_allowlist.items():
            if not isinstance(file_path, str):
                schema_errors.append(
                    f"allowlist schema error: '{config_file}' file path key must be string"
                )
                continue
            if not isinstance(entries, list):
                schema_errors.append(
                    f"allowlist schema error: '{config_file}' entries for '{file_path}' must be list"
                )
                continue

            for entry in entries:
                if not isinstance(entry, dict):
                    schema_errors.append(
                        f"allowlist schema error: '{config_file}' entry for '{file_path}' must be object"
                    )
                    continue

                header = entry.get("header")
                owner = entry.get("owner")
                reason = entry.get("reason")
                window = entry.get("window")
                tier = entry.get("tier")

                if not isinstance(header, str) or not header:
                    schema_errors.append(
                        f"allowlist schema error: '{config_file}' entry for '{file_path}' missing header"
                    )
                    continue
                if not isinstance(owner, str) or not owner.strip():
                    schema_errors.append(
                        f"allowlist schema error: '{file_path}' -> '{header}' missing owner"
                    )
                if not isinstance(reason, str) or not reason.strip():
                    schema_errors.append(
                        f"allowlist schema error: '{file_path}' -> '{header}' missing reason"
                    )
                if not isinstance(window, str) or not window.strip():
                    schema_errors.append(
                        f"allowlist schema error: '{file_path}' -> '{header}' missing window"
                    )
                if not isinstance(tier, str) or tier not in valid_tiers:
                    schema_errors.append(
                        f"allowlist schema error: '{file_path}' -> '{header}' tier must be one of {sorted(valid_tiers)}"
                    )
                    tier = "replaceable"

                key = (file_path.replace("\\", "/"), header)
                if key in allowed:
                    schema_errors.append(
                        f"allowlist schema error: duplicate entry for '{file_path}' -> '{header}'"
                    )
                    continue
                allowed.add(key)
                tier_map[key] = tier

    return allowed, schema_errors, tier_map
