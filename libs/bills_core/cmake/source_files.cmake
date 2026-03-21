set(CONFIG_DIR            "${CORE_SOURCE_ROOT}/config")
set(COMMON_DIR           "${CORE_SOURCE_ROOT}/common")
set(INGEST_DIR            "${CORE_SOURCE_ROOT}/ingest")
set(QUERY_DIR             "${CORE_SOURCE_ROOT}/query")
set(REPORTING_DIR         "${CORE_SOURCE_ROOT}/reporting")
set(ABI_DIR               "${CORE_SOURCE_ROOT}/abi")
set(MODULES_DIR           "${CORE_SOURCE_ROOT}/modules")
set(RECORD_TEMPLATE_DIR   "${CORE_SOURCE_ROOT}/record_template")

set(CONFIG_SOURCES
    "${CONFIG_DIR}/config_bundle_service.cpp"
)

set(INGEST_SOURCES
    "${INGEST_DIR}/bill_workflow_service.cpp"
    "${INGEST_DIR}/pipeline/bills_processing_pipeline.cpp"
    "${INGEST_DIR}/convert/bills_converter.cpp"
    "${INGEST_DIR}/transform/bills_content_transformer.cpp"
    "${INGEST_DIR}/transform/bills_parser.cpp"
    "${INGEST_DIR}/transform/bills_processor.cpp"
    "${INGEST_DIR}/validation/bills_validator.cpp"
    "${INGEST_DIR}/validation/bills_config.cpp"
    "${INGEST_DIR}/validation/validation_result.cpp"
    "${INGEST_DIR}/json/bills_json_serializer.cpp"
)

set(COMMON_SOURCES
    "${COMMON_DIR}/iso_period.cpp"
    "${COMMON_DIR}/text_normalizer.cpp"
)

set(QUERY_SOURCES
    "${QUERY_DIR}/query_service.cpp"
)

set(REPORTING_SOURCES
    "${REPORTING_DIR}/report_render_service.cpp"
    "${REPORTING_DIR}/renderers/standard_report_renderer_registry.cpp"
    "${REPORTING_DIR}/renderers/standard_json_latex_renderer.cpp"
    "${REPORTING_DIR}/renderers/standard_json_markdown_renderer.cpp"
    "${REPORTING_DIR}/renderers/standard_json_rst_renderer.cpp"
    "${REPORTING_DIR}/renderers/standard_json_typst_renderer.cpp"
    "${REPORTING_DIR}/standard_report/standard_report_assembler.cpp"
    "${REPORTING_DIR}/standard_report/standard_report_json_serializer.cpp"
    "${REPORTING_DIR}/sorters/report_sorter.cpp"
)

set(RECORD_TEMPLATE_SOURCES
    "${RECORD_TEMPLATE_DIR}/import_preflight_service.cpp"
    "${RECORD_TEMPLATE_DIR}/ordered_template_layout_loader.cpp"
    "${RECORD_TEMPLATE_DIR}/period_support.cpp"
    "${RECORD_TEMPLATE_DIR}/record_template_service.cpp"
    "${RECORD_TEMPLATE_DIR}/template_render_support.cpp"
)

set(ABI_SOURCES
    "${ABI_DIR}/bills_core_abi.cpp"
)

set(CORE_SOURCES
    ${COMMON_SOURCES}
    ${CONFIG_SOURCES}
    ${INGEST_SOURCES}
    ${QUERY_SOURCES}
    ${REPORTING_SOURCES}
    ${RECORD_TEMPLATE_SOURCES}
    ${ABI_SOURCES}
)

set(CORE_MODULE_INTERFACE_FILES
    "${MODULES_DIR}/core_build_info.cppm"
    "${MODULES_DIR}/common_result.cppm"
    "${MODULES_DIR}/common_process_stats.cppm"
    "${MODULES_DIR}/common_text_normalizer.cppm"
    "${MODULES_DIR}/common_version.cppm"
    "${MODULES_DIR}/domain_bill_record.cppm"
    "${MODULES_DIR}/config_document_types.cppm"
    "${MODULES_DIR}/config_bundle_service.cppm"
    "${MODULES_DIR}/ingest_bill_workflow_service.cppm"
    "${MODULES_DIR}/ingest_bill_processing_pipeline.cppm"
    "${MODULES_DIR}/ingest_bill_json_serializer.cppm"
    "${MODULES_DIR}/query_query_service.cppm"
    "${MODULES_DIR}/reporting_render_service.cppm"
    "${MODULES_DIR}/reporting_renderer_registry.cppm"
    "${MODULES_DIR}/reporting_standard_report_assembler.cppm"
    "${MODULES_DIR}/reporting_standard_report_json_serializer.cppm"
    "${MODULES_DIR}/ports_bills_repository.cppm"
    "${MODULES_DIR}/ports_report_data_gateway.cppm"
    "${MODULES_DIR}/record_template_types.cppm"
    "${MODULES_DIR}/record_template_service.cppm"
    "${MODULES_DIR}/abi_entry.cppm"
)

set(CORE_MODULE_CONSUMER_SOURCES
    "${MODULES_DIR}/core_build_info_import_smoke.cpp"
    "${MODULES_DIR}/core_public_import_smoke.cpp"
    "${MODULES_DIR}/ingest_query_reporting_import_smoke.cpp"
    "${MODULES_DIR}/abi_entry_import_smoke.cpp"
)
