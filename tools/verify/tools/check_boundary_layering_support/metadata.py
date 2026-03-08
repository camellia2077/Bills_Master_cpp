from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

from .models import IncludeRecord
from .policy import classify_entry, reason_for_entry
from .scan import BOUNDARY_LAYER_ROOTS, build_observed_quoted_includes


def normalize_config_path(repo_root: Path, config_arg: str) -> Path:
    config_path = Path(config_arg)
    if config_path.is_absolute():
        return config_path
    return (repo_root / config_path).resolve()


def existing_metadata(config_path: Path) -> dict[tuple[str, str], dict[str, str]]:
    if not config_path.exists():
        return {}
    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}

    result: dict[tuple[str, str], dict[str, str]] = {}
    raw_allowlist = content.get("allowed_quoted_includes", {})
    if not isinstance(raw_allowlist, dict):
        return {}

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


def write_baseline(
    config_path: Path,
    include_records: list[IncludeRecord],
    baseline_owner: str,
) -> None:
    observed = build_observed_quoted_includes(include_records)
    preserved = existing_metadata(config_path)

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

    payload = {
        "schema_version": 1,
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "policy": (
            "Boundary layer allows quoted includes, but each entry must define "
            "owner/reason/window/tier (replaceable vs long-term)."
        ),
        "boundary_layer_roots": BOUNDARY_LAYER_ROOTS,
        "allowed_quoted_includes": ordered_allowlist,
    }

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    total_entries = sum(len(items) for items in ordered_allowlist.values())
    print(
        "[OK] boundary include allowlist baseline generated: "
        f"{config_path} (entries={total_entries})"
    )


def load_allowlist(
    config_path: Path,
) -> tuple[set[tuple[str, str]], list[str], dict[tuple[str, str], str]]:
    if not config_path.exists():
        return set(), [f"Allowlist config does not exist: {config_path}"], {}

    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return set(), [f"Invalid allowlist config JSON: {config_path} ({exc})"], {}

    allowed = set()
    errors: list[str] = []
    tier_map: dict[tuple[str, str], str] = {}
    raw_allowlist = content.get("allowed_quoted_includes")
    if not isinstance(raw_allowlist, dict):
        return (
            set(),
            ["allowlist schema error: 'allowed_quoted_includes' must be object"],
            {},
        )

    valid_tiers = {"replaceable", "long-term"}
    for file_path, entries in raw_allowlist.items():
        if not isinstance(file_path, str):
            errors.append("allowlist schema error: file path key must be string")
            continue
        if not isinstance(entries, list):
            errors.append(f"allowlist schema error: entries for '{file_path}' must be list")
            continue

        for entry in entries:
            if not isinstance(entry, dict):
                errors.append(f"allowlist schema error: entry for '{file_path}' must be object")
                continue

            header = entry.get("header")
            owner = entry.get("owner")
            reason = entry.get("reason")
            window = entry.get("window")
            tier = entry.get("tier")

            if not isinstance(header, str) or not header:
                errors.append(f"allowlist schema error: entry for '{file_path}' missing header")
                continue
            if not isinstance(owner, str) or not owner.strip():
                errors.append(f"allowlist schema error: '{file_path}' -> '{header}' missing owner")
            if not isinstance(reason, str) or not reason.strip():
                errors.append(f"allowlist schema error: '{file_path}' -> '{header}' missing reason")
            if not isinstance(window, str) or not window.strip():
                errors.append(f"allowlist schema error: '{file_path}' -> '{header}' missing window")
            if not isinstance(tier, str) or tier not in valid_tiers:
                errors.append(
                    f"allowlist schema error: '{file_path}' -> '{header}' tier must be one of {sorted(valid_tiers)}"
                )
                tier = "replaceable"

            key = (file_path.replace("\\", "/"), header)
            allowed.add(key)
            tier_map[key] = tier

    return allowed, errors, tier_map
