set(APPLICATION_DIR       "${CORE_SOURCE_ROOT}/application")
set(CONFIG_VALIDATOR_DIR  "${CORE_SOURCE_ROOT}/config_validator")
set(CONVERSION_DIR        "${CORE_SOURCE_ROOT}/billing/conversion")
set(SERIALIZATION_DIR     "${CORE_SOURCE_ROOT}/serialization")
set(REPORTS_DIR           "${CORE_SOURCE_ROOT}/reports")
set(ABI_DIR               "${CORE_SOURCE_ROOT}/abi")
set(MODULES_DIR           "${CORE_SOURCE_ROOT}/modules")

set(CONFIG_VALIDATOR_SOURCES
    "${CONFIG_VALIDATOR_DIR}/pipeline/validator_config_validator.cpp"
    "${CONFIG_VALIDATOR_DIR}/pipeline/modifier_config_validator.cpp"
)

set(CONVERSION_SOURCES
    "${CONVERSION_DIR}/bills_processing_pipeline.cpp"
    "${CONVERSION_DIR}/convert/bills_converter.cpp"
    "${CONVERSION_DIR}/modifier/processor/bills_content_transformer.cpp"
    "${CONVERSION_DIR}/modifier/processor/bills_parser.cpp"
    "${CONVERSION_DIR}/modifier/processor/bills_processor.cpp"
    "${CONVERSION_DIR}/validator/bills_validator.cpp"
    "${CONVERSION_DIR}/validator/config/bills_config.cpp"
    "${CONVERSION_DIR}/validator/result/validation_result.cpp"
)

set(SERIALIZATION_SOURCES
    "${SERIALIZATION_DIR}/bills_json_serializer.cpp"
)

set(REPORTS_SOURCES
    "${REPORTS_DIR}/standard_json/standard_report_assembler.cpp"
    "${REPORTS_DIR}/standard_json/standard_report_json_serializer.cpp"
)

set(ABI_SOURCES
    "${ABI_DIR}/bills_core_abi.cpp"
    "${ABI_DIR}/internal/abi_shared.cpp"
    "${ABI_DIR}/internal/handlers/validate_handler.cpp"
    "${ABI_DIR}/internal/handlers/convert_handler.cpp"
    "${ABI_DIR}/internal/handlers/ingest_import_handler.cpp"
    "${ABI_DIR}/internal/handlers/query_handler.cpp"
)

set(CORE_SOURCES
    ${CONFIG_VALIDATOR_SOURCES}
    ${CONVERSION_SOURCES}
    ${SERIALIZATION_SOURCES}
    ${REPORTS_SOURCES}
    ${ABI_SOURCES}
)

set(CORE_MODULE_INTERFACE_FILES
    "${MODULES_DIR}/core_build_info.cppm"
    "${MODULES_DIR}/common_result.cppm"
    "${MODULES_DIR}/common_process_stats.cppm"
    "${MODULES_DIR}/common_version.cppm"
    "${MODULES_DIR}/domain_bill_record.cppm"
    "${MODULES_DIR}/config_validator_validator_types.cppm"
    "${MODULES_DIR}/config_validator_validator.cppm"
    "${MODULES_DIR}/config_validator_modifier_types.cppm"
    "${MODULES_DIR}/config_validator_modifier.cppm"
    "${MODULES_DIR}/config_bill_config.cppm"
    "${MODULES_DIR}/config_modifier_data.cppm"
    "${MODULES_DIR}/serialization_bill_json_serializer.cppm"
    "${MODULES_DIR}/billing_processing_pipeline.cppm"
    "${MODULES_DIR}/ports_bills_content_reader.cppm"
    "${MODULES_DIR}/ports_bills_file_enumerator.cppm"
    "${MODULES_DIR}/ports_bills_repository.cppm"
    "${MODULES_DIR}/ports_bills_serializer.cppm"
    "${MODULES_DIR}/ports_output_path_builder.cppm"
    "${MODULES_DIR}/ports_month_report_formatter_provider.cppm"
    "${MODULES_DIR}/ports_yearly_report_formatter_provider.cppm"
    "${MODULES_DIR}/ports_report_data_gateway.cppm"
    "${MODULES_DIR}/reports_standard_report_assembler.cppm"
    "${MODULES_DIR}/reports_standard_report_json_serializer.cppm"
    "${MODULES_DIR}/reports_report_exporter.cppm"
    "${MODULES_DIR}/application_workflow_use_case.cppm"
    "${MODULES_DIR}/abi_shared.cppm"
)

set(CORE_MODULE_CONSUMER_SOURCES
    "${MODULES_DIR}/core_build_info_import_smoke.cpp"
    "${MODULES_DIR}/phase2_core_module_import_smoke.cpp"
    "${MODULES_DIR}/phase2_call_chain_module_import_smoke.cpp"
    "${MODULES_DIR}/phase3_layer_module_import_smoke.cpp"
    "${MODULES_DIR}/phase3_abi_module_import_smoke.cpp"
)
