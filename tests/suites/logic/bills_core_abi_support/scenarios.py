from __future__ import annotations

import tempfile
from pathlib import Path

from .assertions import require, require_response_envelope
from .client import AbiClient
from .fixtures import parse_iso_month_from_stem, select_fixture_txt_path
from .models import AbiSchemaContext

REPO_ROOT = Path(__file__).resolve().parents[4]
UTF8_BOM = b"\xef\xbb\xbf"


def _canonicalize_bill_bytes(raw_bytes: bytes) -> bytes:
    text = raw_bytes.decode("utf-8-sig")
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    return text.encode("utf-8")


def _write_bill_bytes(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def run_text_normalizer_compatibility_tests(
    client: AbiClient,
    schema: AbiSchemaContext,
    fixture_config_dir: Path,
    fixture_txt: Path,
) -> None:
    canonical_bytes = _canonicalize_bill_bytes(fixture_txt.read_bytes())
    valid_variants = [
        ("lf", canonical_bytes),
        ("crlf", canonical_bytes.replace(b"\n", b"\r\n")),
        ("utf8_bom_lf", UTF8_BOM + canonical_bytes),
        ("utf8_bom_crlf", UTF8_BOM + canonical_bytes.replace(b"\n", b"\r\n")),
    ]

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_text_") as temp_dir:
        temp_root = Path(temp_dir)

        for name, payload in valid_variants:
            input_path = temp_root / name / fixture_txt.name
            output_dir = temp_root / name / "txt2json"
            _write_bill_bytes(input_path, payload)

            validate = client.invoke(
                {
                    "command": "validate",
                    "params": {
                        "input_path": str(input_path),
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
            require(
                validate_data.get("all_valid") is True,
                f"{name}: validate.all_valid must be true.",
            )

            convert = client.invoke(
                {
                    "command": "convert",
                    "params": {
                        "input_path": str(input_path),
                        "output_dir": str(output_dir),
                        "write_files": False,
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
            require(
                convert_data.get("all_converted") is True,
                f"{name}: convert.all_converted must be true.",
            )

            ingest = client.invoke(
                {
                    "command": "ingest",
                    "params": {
                        "input_path": str(input_path),
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
            require(
                ingest_data.get("all_ingested") is True,
                f"{name}: ingest.all_ingested must be true.",
            )

        invalid_input = temp_root / "invalid_utf8" / fixture_txt.name
        _write_bill_bytes(invalid_input, canonical_bytes + b"\xff")

        invalid_commands = [
            (
                "validate",
                "business.validation_failed",
                {
                    "input_path": str(invalid_input),
                    "config_dir": str(fixture_config_dir),
                },
            ),
            (
                "convert",
                "business.convert_failed",
                {
                    "input_path": str(invalid_input),
                    "output_dir": str(temp_root / "invalid_utf8" / "txt2json"),
                    "write_files": False,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
                },
            ),
            (
                "ingest",
                "business.ingest_failed",
                {
                    "input_path": str(invalid_input),
                    "output_dir": str(temp_root / "invalid_utf8" / "txt2json"),
                    "write_json": False,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
                },
            ),
        ]

        for command, expected_code, params in invalid_commands:
            response = client.invoke({"command": command, "params": params})
            require_response_envelope(
                response,
                schema,
                expected_ok=False,
                expected_code=expected_code,
                expected_error_layer="business",
            )
            files = response.get("data", {}).get("files", [])
            require(
                isinstance(files, list) and len(files) == 1,
                f"{command}: expected one file result.",
            )
            require(files[0].get("ok") is False, f"{command}: file result must fail.")
            error_text = files[0].get("error", "")
            require(
                isinstance(error_text, str) and "UTF-8" in error_text,
                f"{command}: file error must mention UTF-8.",
            )

    print("[PASS] text normalizer compatibility and invalid UTF-8 handling.")


def run_record_template_command_tests(
    client: AbiClient,
    schema: AbiSchemaContext,
    fixture_config_dir: Path,
    fixture_txt: Path,
    iso_month: str,
) -> None:
    inspect = client.invoke(
        {
            "command": "config_inspect",
            "params": {
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        inspect,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    inspect_data = inspect.get("data", {})
    require(inspect_data.get("schema_version") == 1, "config_inspect schema_version mismatch.")
    require(inspect_data.get("date_format") == "YYYY-MM", "config_inspect date_format mismatch.")
    require(
        inspect_data.get("metadata_headers") == ["date", "remark"],
        "config_inspect metadata_headers mismatch.",
    )
    categories = inspect_data.get("categories", [])
    require(isinstance(categories, list) and categories, "config_inspect categories missing.")
    require(categories[0].get("parent_item") == "meal", "config_inspect first parent mismatch.")
    require(
        categories[0].get("description") == "Meal expenses",
        "config_inspect first description mismatch.",
    )
    require(
        categories[0].get("sub_items", [])[:3] == ["meal_low", "meal_high", "meal_snacks"],
        "config_inspect sub-item order mismatch.",
    )
    print("[PASS] config inspect command.")

    single_template = client.invoke(
        {
            "command": "template_generate",
            "params": {
                "period": "2026-03",
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        single_template,
        schema,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
    )
    single_template_data = single_template.get("data", {})
    require(single_template_data.get("generated") == 1, "single template count mismatch.")
    templates = single_template_data.get("templates", [])
    require(isinstance(templates, list) and len(templates) == 1, "single template missing.")
    template_item = templates[0]
    require(
        template_item.get("relative_path") == "2026/2026-03.txt",
        "single template relative_path mismatch.",
    )
    template_text = template_item.get("text", "")
    require(template_text.startswith("date:2026-03\nremark:\n\n"), "template header mismatch.")
    require(template_text.find("meal") < template_text.find("fitness"), "parent order mismatch.")
    require(
        template_text.find("meal_low") < template_text.find("meal_high"),
        "sub-item order mismatch.",
    )
    print("[PASS] template_generate single period.")

    invalid_period = client.invoke(
        {
            "command": "template_generate",
            "params": {
                "period": "2026",
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        invalid_period,
        schema,
        expected_ok=False,
        expected_code="param.invalid_request",
        expected_error_layer="param",
    )

    reversed_range = client.invoke(
        {
            "command": "template_generate",
            "params": {
                "start_period": "2026-03",
                "end_period": "2026-01",
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        reversed_range,
        schema,
        expected_ok=False,
        expected_code="param.invalid_request",
        expected_error_layer="param",
    )
    print("[PASS] template_generate invalid period validation.")

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_templates_") as temp_dir:
        output_dir = Path(temp_dir) / "generated_templates"

        month_range = client.invoke(
            {
                "command": "template_generate",
                "params": {
                    "start_period": "2026-01",
                    "end_period": "2026-03",
                    "config_dir": str(fixture_config_dir),
                    "write_files": True,
                    "output_dir": str(output_dir),
                },
            }
        )
        require_response_envelope(
            month_range,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        month_range_data = month_range.get("data", {})
        month_templates = month_range_data.get("templates", [])
        require(
            isinstance(month_templates, list) and len(month_templates) == 3,
            "month range template count mismatch.",
        )
        expected_relpaths = [
            "2026/2026-01.txt",
            "2026/2026-02.txt",
            "2026/2026-03.txt",
        ]
        require(
            [item.get("relative_path") for item in month_templates] == expected_relpaths,
            "month range relative paths mismatch.",
        )
        for template in month_templates:
            output_path = Path(template.get("output_path", ""))
            require(output_path.is_file(), f"template output missing: {output_path}")

        year_range = client.invoke(
            {
                "command": "template_generate",
                "params": {
                    "start_year": "2025",
                    "end_year": "2026",
                    "config_dir": str(fixture_config_dir),
                },
            }
        )
        require_response_envelope(
            year_range,
            schema,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
        )
        year_templates = year_range.get("data", {}).get("templates", [])
        require(
            isinstance(year_templates, list) and len(year_templates) == 24,
            "year range template count mismatch.",
        )
        require(year_templates[0].get("period") == "2025-01", "year range first period mismatch.")
        require(year_templates[-1].get("period") == "2026-12", "year range last period mismatch.")
        print("[PASS] template_generate month/year range expansion.")

        preview_root = Path(temp_dir) / "preview"
        valid_a = preview_root / "a.txt"
        valid_b = preview_root / "nested" / "duplicate.txt"
        invalid = preview_root / "bad.txt"
        valid_a.parent.mkdir(parents=True, exist_ok=True)
        valid_b.parent.mkdir(parents=True, exist_ok=True)
        valid_a.write_bytes(fixture_txt.read_bytes())
        valid_b.write_bytes(fixture_txt.read_bytes())
        invalid.write_text("date:2025/01\nremark:\n", encoding="utf-8")

        periods = client.invoke(
            {
                "command": "list_periods",
                "params": {
                    "input_path": str(preview_root),
                },
            }
        )
        require_response_envelope(
            periods,
            schema,
            expected_ok=False,
            expected_code="business.validation_failed",
            expected_error_layer="business",
        )
        periods_data = periods.get("data", {})
        require(periods_data.get("processed") == 3, "list_periods processed mismatch.")
        require(periods_data.get("valid") == 2, "list_periods valid mismatch.")
        require(periods_data.get("invalid") == 1, "list_periods invalid mismatch.")
        require(periods_data.get("periods") == [iso_month], "list_periods periods mismatch.")
        invalid_files = periods_data.get("invalid_files", [])
        require(
            isinstance(invalid_files, list) and len(invalid_files) == 1,
            "list_periods invalid_files mismatch.",
        )
        require(
            Path(invalid_files[0].get("path", "")) == invalid,
            "list_periods invalid path mismatch.",
        )

        preview = client.invoke(
            {
                "command": "record_preview",
                "params": {
                    "input_path": str(preview_root),
                    "config_dir": str(fixture_config_dir),
                },
            }
        )
        require_response_envelope(
            preview,
            schema,
            expected_ok=False,
            expected_code="business.validation_failed",
            expected_error_layer="business",
        )
        preview_data = preview.get("data", {})
        require(preview_data.get("processed") == 3, "record_preview processed mismatch.")
        require(preview_data.get("success") == 2, "record_preview success mismatch.")
        require(preview_data.get("failure") == 1, "record_preview failure mismatch.")
        require(preview_data.get("periods") == [iso_month], "record_preview periods mismatch.")
        preview_files = preview_data.get("files", [])
        require(isinstance(preview_files, list) and len(preview_files) == 3, "record_preview files mismatch.")
        valid_items = [item for item in preview_files if item.get("ok") is True]
        require(len(valid_items) == 2, "record_preview valid file count mismatch.")
        require(
            all(item.get("period") == iso_month for item in valid_items),
            "record_preview period extraction mismatch.",
        )
        require(
            all(int(item.get("transaction_count", 0)) > 0 for item in valid_items),
            "record_preview transaction_count must be > 0 for valid files.",
        )
        failed_items = [item for item in preview_files if item.get("ok") is False]
        require(len(failed_items) == 1, "record_preview failed file count mismatch.")
        require(
            isinstance(failed_items[0].get("error"), str) and failed_items[0]["error"],
            "record_preview failed file must include error.",
        )
        print("[PASS] list_periods and record_preview commands.")


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

        invalid_year = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "year",
                    "value": iso_month,
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            invalid_year,
            schema,
            expected_ok=False,
            expected_code="param.invalid_request",
            expected_error_layer="param",
        )
        invalid_year_data = invalid_year.get("data", {})
        require(
            invalid_year_data.get("expected_format") == "YYYY",
            "query year invalid format should report YYYY.",
        )

        invalid_month = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "month",
                    "value": f"{year}-1",
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            invalid_month,
            schema,
            expected_ok=False,
            expected_code="param.invalid_request",
            expected_error_layer="param",
        )
        invalid_month_data = invalid_month.get("data", {})
        require(
            invalid_month_data.get("expected_format") == "YYYY-MM",
            "query month invalid format should report YYYY-MM.",
        )
        print("[PASS] query period format validation.")

    run_text_normalizer_compatibility_tests(
        client,
        schema,
        fixture_config_dir,
        fixture_txt,
    )
    run_record_template_command_tests(
        client,
        schema,
        fixture_config_dir,
        fixture_txt,
        iso_month,
    )


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
        "template_generate",
        "record_preview",
        "config_inspect",
        "list_periods",
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
