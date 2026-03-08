from __future__ import annotations

import tempfile
from pathlib import Path

from .assertions import require, require_response_envelope
from .client import AbiClient
from .fixtures import parse_iso_month_from_stem, select_fixture_txt_path
from .models import AbiSchemaContext

REPO_ROOT = Path(__file__).resolve().parents[4]


def run_fixture_command_tests(
    client: AbiClient,
    schema: AbiSchemaContext,
) -> None:
    fixture_config_dir = REPO_ROOT / "tests" / "config"
    require(fixture_config_dir.is_dir(), f"Fixture config dir missing: {fixture_config_dir}")

    fixture_txt = select_fixture_txt_path()
    year, month, iso_month = parse_iso_month_from_stem(fixture_txt)

    validate = client.invoke(
        {
            "command": "validate",
            "params": {
                "input_path": str(fixture_txt),
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        validate,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    validate_data = validate.get("data", {})
    require(validate_data.get("processed") == 1, "validate.processed must be 1.")
    require(validate_data.get("all_valid") is True, "validate.all_valid must be true.")
    print("[PASS] validate fixture.")

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_") as temp_dir:
        output_dir = Path(temp_dir) / "txt2json"

        convert = client.invoke(
            {
                "command": "convert",
                "params": {
                    "input_path": str(fixture_txt),
                    "output_dir": str(output_dir),
                    "write_files": True,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
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
        convert_data = convert.get("data", {})
        require(convert_data.get("processed") == 1, "convert.processed must be 1.")
        require(convert_data.get("all_converted") is True, "convert.all_converted must be true.")

        convert_files = convert_data.get("files", [])
        require(
            isinstance(convert_files, list) and len(convert_files) == 1,
            "convert.files must have 1 item.",
        )
        output_path_raw = convert_files[0].get("output_path", "")
        require(
            isinstance(output_path_raw, str) and output_path_raw,
            "convert.files[0].output_path must exist.",
        )
        output_path = Path(output_path_raw)
        require(output_path.is_file(), f"convert output file must exist: {output_path}")
        print("[PASS] convert fixture and wrote JSON.")

        ingest = client.invoke(
            {
                "command": "ingest",
                "params": {
                    "input_path": str(fixture_txt),
                    "output_dir": str(output_dir),
                    "write_json": False,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
                },
            }
        )
        require_response_envelope(
            ingest,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        ingest_data = ingest.get("data", {})
        require(ingest_data.get("processed") == 1, "ingest.processed must be 1.")
        require(ingest_data.get("all_ingested") is True, "ingest.all_ingested must be true.")
        require(ingest_data.get("imported") == 1, "ingest.imported must be 1.")
        require(
            ingest_data.get("repository_mode") == "memory",
            "ingest.repository_mode must be 'memory'.",
        )
        print("[PASS] ingest fixture.")

        imported = client.invoke(
            {
                "command": "import",
                "params": {
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            imported,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        import_data = imported.get("data", {})
        require(import_data.get("processed") == 1, "import.processed must be 1.")
        require(import_data.get("all_imported") is True, "import.all_imported must be true.")
        require(import_data.get("imported") == 1, "import.imported must be 1.")
        require(
            import_data.get("repository_mode") == "memory",
            "import.repository_mode must be 'memory'.",
        )

        import_files = import_data.get("files", [])
        require(
            isinstance(import_files, list) and len(import_files) == 1,
            "import.files must have 1 item.",
        )
        require(import_files[0].get("ok") is True, "import.files[0].ok must be true.")
        require(import_files[0].get("year") == year, "import.files[0].year mismatch.")
        require(import_files[0].get("month") == month, "import.files[0].month mismatch.")
        print("[PASS] import fixture.")

        query_year = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "year",
                    "value": str(year),
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            query_year,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        qy_data = query_year.get("data", {})
        require(qy_data.get("query_type") == "year", "query_year.query_type must be 'year'.")
        require(qy_data.get("year") == year, "query_year.year mismatch.")
        require(qy_data.get("processed") == 1, "query_year.processed must be 1.")
        require(qy_data.get("matched_bills") == 1, "query_year.matched_bills must be 1.")
        require("monthly_summary" in qy_data, "query_year.monthly_summary must exist.")
        print("[PASS] query year fixture.")

        query_month = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "month",
                    "value": iso_month,
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            query_month,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        qm_data = query_month.get("data", {})
        require(qm_data.get("query_type") == "month", "query_month.query_type must be 'month'.")
        require(qm_data.get("year") == year, "query_month.year mismatch.")
        require(qm_data.get("month") == month, "query_month.month mismatch.")
        require(qm_data.get("processed") == 1, "query_month.processed must be 1.")
        require(qm_data.get("matched_bills") == 1, "query_month.matched_bills must be 1.")
        require("monthly_summary" not in qm_data, "query_month.monthly_summary must not exist.")
        print("[PASS] query month fixture.")


def run_suite(client: AbiClient, smoke_loops: int) -> None:
    abi_version = client.abi_version()
    require(bool(abi_version), "abi_version must be non-empty.")
    print("[PASS] abi_version is non-empty.")

    caps = client.capabilities()
    require(caps.get("abi_version") == abi_version, "capabilities.abi_version mismatch.")
    response_schema_version = caps.get("response_schema_version")
    error_code_schema_version = caps.get("error_code_schema_version")
    require(
        isinstance(response_schema_version, int),
        "capabilities.response_schema_version must be int.",
    )
    require(
        isinstance(error_code_schema_version, int),
        "capabilities.error_code_schema_version must be int.",
    )
    commands = caps.get("supported_commands", [])
    require(isinstance(commands, list), "capabilities.supported_commands must be list.")
    for required_command in [
        "version",
        "capabilities",
        "ping",
        "validate",
        "convert",
        "ingest",
        "import",
        "query",
    ]:
        require(required_command in commands, f"Missing command in capabilities: {required_command}")
    require(
        caps.get("error_layers") == ["none", "param", "business", "system"],
        "capabilities.error_layers mismatch.",
    )
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
    version_data = version.get("data", {})
    require(version_data.get("abi_version") == abi_version, "version.abi_version mismatch.")
    require(
        version_data.get("response_schema_version") == response_schema_version,
        "version.data.response_schema_version mismatch.",
    )
    require(
        version_data.get("error_code_schema_version") == error_code_schema_version,
        "version.data.error_code_schema_version mismatch.",
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
    print("[PASS] null request handling.")

    invalid_json = client.invoke_text("{")
    require_response_envelope(
        invalid_json,
        schema,
        expected_ok=False,
        expected_code="param.invalid_json",
        expected_error_layer="param",
    )
    print("[PASS] invalid JSON handling.")

    unknown = client.invoke({"command": "nonexistent_command"})
    require_response_envelope(
        unknown,
        schema,
        expected_ok=False,
        expected_code="param.unknown_command",
        expected_error_layer="param",
    )
    print("[PASS] unknown command handling.")

    invalid_params = client.invoke({"command": "validate", "params": []})
    require_response_envelope(
        invalid_params,
        schema,
        expected_ok=False,
        expected_code="param.invalid_request",
        expected_error_layer="param",
    )
    print("[PASS] invalid params type handling.")

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_empty_") as empty_dir:
        business = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "year",
                    "value": "2024",
                    "input_path": str(Path(empty_dir)),
                },
            }
        )
    require_response_envelope(
        business,
        schema,
        expected_ok=False,
        expected_code="business.no_input_files",
        expected_error_layer="business",
    )
    print("[PASS] business error layer handling.")

    ping_payload = {"hello": "world", "n": 7}
    ping = client.invoke({"command": "ping", "payload": ping_payload})
    require_response_envelope(
        ping,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    require(ping.get("data", {}).get("pong") is True, "ping response must contain pong=true.")
    require(
        ping.get("data", {}).get("echo") == ping_payload,
        "ping echo payload mismatch.",
    )
    print("[PASS] ping echo contract.")

    run_fixture_command_tests(client, schema)

    for _ in range(smoke_loops):
        _ = client.capabilities()
        _ = client.invoke({"command": "version"})
        _ = client.invoke({"command": "ping", "payload": {"loop": True}})
        _ = client.invoke_text("{")
        _ = client.invoke({"command": "nonexistent_command"})
    print(f"[PASS] ownership/free smoke test ({smoke_loops} loops, mixed responses).")
