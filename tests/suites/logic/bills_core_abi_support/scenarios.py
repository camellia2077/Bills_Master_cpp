from __future__ import annotations

import tomllib
from pathlib import Path

from .assertions import require, require_response_envelope
from .client import AbiClient
from .fixtures import parse_iso_month_from_stem, select_fixture_txt_path
from .models import AbiSchemaContext

REPO_ROOT = Path(__file__).resolve().parents[4]


def _load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _load_validator_document(config_dir: Path) -> dict:
    data = tomllib.loads(_load_text(config_dir / "validator_config.toml"))
    return {
        "display_path": "validator_config.toml",
        "categories": data.get("categories", []),
    }


def _load_modifier_document(config_dir: Path) -> dict:
    data = tomllib.loads(_load_text(config_dir / "modifier_config.toml"))
    return {
        "display_path": "modifier_config.toml",
        "auto_renewal_rules": data.get("auto_renewal_rules", {}),
        "metadata_prefixes": data.get("metadata_prefixes", []),
        "display_name_maps": data.get("display_name_maps", {}),
    }


def _load_export_formats_document(config_dir: Path) -> dict:
    data = tomllib.loads(_load_text(config_dir / "export_formats.toml"))
    return {
        "display_path": "export_formats.toml",
        "enabled_formats": data.get("enabled_formats", []),
    }


def _config_payload(config_dir: Path) -> dict:
    return {
        "validator_document": _load_validator_document(config_dir),
        "modifier_document": _load_modifier_document(config_dir),
        "export_formats_document": _load_export_formats_document(config_dir),
    }


def _text_documents(paths: list[Path]) -> list[dict]:
    return [{"display_path": path.name, "text": _load_text(path)} for path in paths]


def _json_documents_from_convert(convert_response: dict) -> list[dict]:
    documents: list[dict] = []
    for item in convert_response.get("data", {}).get("files", []):
        serialized = item.get("serialized_json", "")
        if item.get("ok") is True and isinstance(serialized, str) and serialized:
            display_path = Path(str(item.get("path", ""))).with_suffix(".json").name
            documents.append({"display_path": display_path, "text": serialized})
    return documents


def run_suite(client: AbiClient, smoke_loops: int) -> None:
    fixture_config_dir = REPO_ROOT / "tests" / "config"
    fixture_txt = select_fixture_txt_path()
    year, month, iso_month = parse_iso_month_from_stem(fixture_txt)
    config_payload = _config_payload(fixture_config_dir)
    documents = _text_documents([fixture_txt])

    abi_version = client.abi_version()
    require(bool(abi_version), "abi_version must be non-empty.")
    print("[PASS] abi_version is non-empty.")

    caps = client.capabilities()
    require(caps.get("abi_version") == abi_version, "capabilities.abi_version mismatch.")
    response_schema_version = caps.get("response_schema_version")
    error_code_schema_version = caps.get("error_code_schema_version")
    require(isinstance(response_schema_version, int), "response_schema_version must be int.")
    require(isinstance(error_code_schema_version, int), "error_code_schema_version must be int.")
    commands = caps.get("supported_commands", [])
    for required_command in [
        "version",
        "capabilities",
        "ping",
        "validate_config_bundle",
        "template_generate",
        "validate_record_batch",
        "preflight_import",
        "validate",
        "convert",
        "ingest",
        "import",
        "query",
    ]:
        require(required_command in commands, f"Missing command in capabilities: {required_command}")
    print("[PASS] capabilities includes required commands.")

    schema = AbiSchemaContext(
        abi_version=abi_version,
        response_schema_version=response_schema_version,
        error_code_schema_version=error_code_schema_version,
    )

    version = client.invoke({"command": "version"})
    require_response_envelope(
        version,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    print("[PASS] version response schema.")

    null_input = client.invoke_null()
    require_response_envelope(
        null_input,
        schema,
        expected_ok=False,
        expected_code="param.invalid_argument",
        expected_error_layer="param",
    )
    invalid_json = client.invoke_text("{")
    require_response_envelope(
        invalid_json,
        schema,
        expected_ok=False,
        expected_code="param.invalid_json",
        expected_error_layer="param",
    )
    unknown = client.invoke({"command": "nonexistent_command"})
    require_response_envelope(
        unknown,
        schema,
        expected_ok=False,
        expected_code="param.unknown_command",
        expected_error_layer="param",
    )
    print("[PASS] null/invalid/unknown command handling.")

    ping_payload = {"hello": "world", "n": 7}
    ping = client.invoke({"command": "ping", "payload": ping_payload})
    require_response_envelope(
        ping,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(ping.get("data", {}).get("echo") == ping_payload, "ping echo mismatch.")
    print("[PASS] ping echo contract.")

    validate_config = client.invoke(
        {
            "command": "validate_config_bundle",
            "params": config_payload,
        }
    )
    require_response_envelope(
        validate_config,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(
        validate_config.get("data", {}).get("enabled_export_formats")
        == ["md", "json", "tex", "rst"],
        "enabled_export_formats mismatch.",
    )
    print("[PASS] validate_config_bundle.")

    validate = client.invoke(
        {
            "command": "validate",
            "params": {**config_payload, "documents": documents},
        }
    )
    require_response_envelope(
        validate,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(validate.get("data", {}).get("success") == 1, "validate success mismatch.")
    print("[PASS] validate document batch.")

    convert = client.invoke(
        {
            "command": "convert",
            "params": {
                **config_payload,
                "documents": documents,
                "include_serialized_json": True,
            },
        }
    )
    require_response_envelope(
        convert,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    json_documents = _json_documents_from_convert(convert)
    require(len(json_documents) == 1, "convert must emit one serialized json document.")
    print("[PASS] convert document batch.")

    ingest = client.invoke(
        {
            "command": "ingest",
            "params": {**config_payload, "documents": documents},
        }
    )
    require_response_envelope(
        ingest,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(ingest.get("data", {}).get("imported") == 1, "ingest imported mismatch.")
    print("[PASS] ingest document batch.")

    imported = client.invoke(
        {
            "command": "import",
            "params": {"documents": json_documents},
        }
    )
    require_response_envelope(
        imported,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(imported.get("data", {}).get("imported") == 1, "import imported mismatch.")
    print("[PASS] import json document batch.")

    query_year = client.invoke(
        {
            "command": "query",
            "params": {"type": "year", "value": str(year), "documents": json_documents},
        }
    )
    require_response_envelope(
        query_year,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(query_year.get("data", {}).get("matched_bills") == 1, "query year mismatch.")

    query_month = client.invoke(
        {
            "command": "query",
            "params": {"type": "month", "value": iso_month, "documents": json_documents},
        }
    )
    require_response_envelope(
        query_month,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(query_month.get("data", {}).get("matched_bills") == 1, "query month mismatch.")
    print("[PASS] query json document batch.")

    invalid_query = client.invoke(
        {
            "command": "query",
            "params": {"type": "year", "value": iso_month, "documents": json_documents},
        }
    )
    require_response_envelope(
        invalid_query,
        schema,
        expected_ok=False,
        expected_code="param.invalid_request",
        expected_error_layer="param",
    )
    print("[PASS] query format validation.")

    template_generate = client.invoke(
        {
            "command": "template_generate",
            "params": {
                "period": "2026-03",
                "validator_document": config_payload["validator_document"],
            },
        }
    )
    require_response_envelope(
        template_generate,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    templates = template_generate.get("data", {}).get("templates", [])
    require(len(templates) == 1, "template_generate count mismatch.")
    require(templates[0].get("relative_path") == "2026/2026-03.txt", "relative_path mismatch.")
    print("[PASS] template_generate.")

    validate_record_batch = client.invoke(
        {
            "command": "validate_record_batch",
            "params": {**config_payload, "documents": documents},
        }
    )
    require_response_envelope(
        validate_record_batch,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(
        validate_record_batch.get("data", {}).get("periods") == [iso_month],
        "validate_record_batch periods mismatch.",
    )
    print("[PASS] validate_record_batch.")

    duplicate_documents = [
        {"display_path": fixture_txt.name, "text": _load_text(fixture_txt)},
        {"display_path": f"copy-{fixture_txt.name}", "text": _load_text(fixture_txt)},
    ]
    preflight = client.invoke(
        {
            "command": "preflight_import",
            "params": {
                **config_payload,
                "documents": duplicate_documents,
                "existing_workspace_periods": [iso_month],
                "existing_db_periods": [iso_month],
            },
        }
    )
    require_response_envelope(
        preflight,
        schema,
        expected_ok=False,
        expected_code="business.validation_failed",
        expected_error_layer="business",
    )
    preflight_data = preflight.get("data", {})
    require(iso_month in preflight_data.get("duplicate_periods", []), "duplicate period missing.")
    require(
        iso_month in preflight_data.get("workspace_conflict_periods", []),
        "workspace conflict missing.",
    )
    require(iso_month in preflight_data.get("db_conflict_periods", []), "db conflict missing.")
    print("[PASS] preflight_import.")

    for _ in range(smoke_loops):
        _ = client.capabilities()
        _ = client.invoke({"command": "version"})
        _ = client.invoke({"command": "ping", "payload": {"loop": True}})
        _ = client.invoke_text("{")
        _ = client.invoke({"command": "nonexistent_command"})
    print(f"[PASS] ownership/free smoke test ({smoke_loops} loops, mixed responses).")
