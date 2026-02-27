set(APPLICATION_DIR       "${CORE_SOURCE_ROOT}/application")
set(CONFIG_VALIDATOR_DIR  "${CORE_SOURCE_ROOT}/config_validator")
set(CONVERSION_DIR        "${CORE_SOURCE_ROOT}/billing/conversion")
set(SERIALIZATION_DIR     "${CORE_SOURCE_ROOT}/serialization")
set(REPORTS_DIR           "${CORE_SOURCE_ROOT}/reports")
set(ABI_DIR               "${CORE_SOURCE_ROOT}/abi")

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
