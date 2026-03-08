from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

from .models import IncludeRecord
from .scan import CALL_LAYER_ROOTS, build_observed_quoted_includes


def normalize_config_path(repo_root: Path, config_arg: str) -> Path:
    config_path = Path(config_arg)
    if config_path.is_absolute():
        return config_path
    return (repo_root / config_path).resolve()


def load_allowlist(
    config_path: Path,
) -> tuple[set[tuple[str, str]], list[str]]:
    if not config_path.exists():
        return set(), [f"Allowlist config does not exist: {config_path}"]

    try:
        content = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return set(), [f"Invalid allowlist config JSON: {config_path} ({exc})"]

    allowed = set()
    errors: list[str] = []
    raw_allowlist = content.get("allowed_quoted_includes")
    if not isinstance(raw_allowlist, dict):
        return set(), ["allowlist schema error: 'allowed_quoted_includes' must be object"]

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
            if not isinstance(header, str) or not header:
                errors.append(f"allowlist schema error: entry for '{file_path}' missing header")
                continue
            if not isinstance(owner, str) or not owner.strip():
                errors.append(f"allowlist schema error: '{file_path}' -> '{header}' missing owner")
            if not isinstance(reason, str) or not reason.strip():
                errors.append(f"allowlist schema error: '{file_path}' -> '{header}' missing reason")

            allowed.add((file_path.replace("\\", "/"), header))

    return allowed, errors


def existing_metadata(
    config_path: Path,
) -> dict[tuple[str, str], dict[str, str]]:
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
            owner = entry.get("owner")
            reason = entry.get("reason")
            result[(normalized_path, header)] = {
                "owner": owner if isinstance(owner, str) and owner.strip() else "",
                "reason": reason if isinstance(reason, str) and reason.strip() else "",
            }
    return result


def write_baseline(
    repo_root: Path,
    config_path: Path,
    include_records: list[IncludeRecord],
    baseline_owner: str,
    baseline_reason: str,
) -> None:
    del repo_root
    observed = build_observed_quoted_includes(include_records)
    preserved = existing_metadata(config_path)

    ordered_allowlist: dict[str, list[dict[str, str]]] = {}
    for file_path in sorted(observed.keys()):
        headers = sorted(observed[file_path])
        items: list[dict[str, str]] = []
        for header in headers:
            meta = preserved.get((file_path, header), {})
            owner = meta.get("owner") or baseline_owner
            reason = meta.get("reason") or baseline_reason
            items.append(
                {
                    "header": header,
                    "owner": owner,
                    "reason": reason,
                }
            )
        ordered_allowlist[file_path] = items

    payload = {
        "schema_version": 1,
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "policy": (
            "Call-layer files should default to import. "
            "Quoted includes must be explicitly allowlisted."
        ),
        "call_layer_roots": CALL_LAYER_ROOTS,
        "allowed_quoted_includes": ordered_allowlist,
    }

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    print(
        "[OK] import-layer allowlist baseline generated: "
        f"{config_path} (entries={sum(len(v) for v in ordered_allowlist.values())})"
    )
