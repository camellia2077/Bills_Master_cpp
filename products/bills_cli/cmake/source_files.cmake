# ==============================================================================
#  源文件收集
# ==============================================================================
set(CORE_SOURCE_ROOT      "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/bills_core/src")
set(CONVERSION_DIR        "${CORE_SOURCE_ROOT}/billing/conversion")
set(CORE_REPORTS_DIR      "${CORE_SOURCE_ROOT}/reports")
set(CLI_REPORTS_DIR       "${SOURCE_ROOT}/reports")
set(CORE_ABI_DIR          "${CORE_SOURCE_ROOT}/abi")
set(APPLICATION_DIR       "${CORE_SOURCE_ROOT}/application")
set(CONFIG_VALIDATOR_DIR  "${CORE_SOURCE_ROOT}/config_validator")
set(CLI_PRESENTATION_DIR  "${SOURCE_ROOT}/windows/presentation/cli")
set(WINDOWS_INFRA_DIR     "${SOURCE_ROOT}/windows/infrastructure")
set(FILEHANDLER_DIR       "${WINDOWS_INFRA_DIR}/file_handler")
set(ADAPTER_CONFIG_DIR    "${WINDOWS_INFRA_DIR}/adapters/config")
set(ADAPTER_IO_DIR        "${WINDOWS_INFRA_DIR}/adapters/io")
set(ADAPTER_DB_DIR        "${WINDOWS_INFRA_DIR}/adapters/db")
set(COMMAND_HANDLER_DIR   "${CLI_PRESENTATION_DIR}/command_handler")
set(CONTROLLERS_DIR       "${CLI_PRESENTATION_DIR}/controllers")



# controllers
set(CONTROLLER_SOURCES
    "${CONTROLLERS_DIR}/app_controller.cpp"
    "${CONTROLLERS_DIR}/export/export_controller.cpp"
    "${CONTROLLERS_DIR}/workflow/workflow_controller.cpp"
    "${CONTROLLERS_DIR}/workflow/path_builder.cpp"
)

set(APPLICATION_SOURCES
    "${APPLICATION_DIR}/use_cases/workflow_use_case.cpp"
)

set(CONFIG_VALIDATOR_SOURCES
    "${CONFIG_VALIDATOR_DIR}/pipeline/validator_config_validator.cpp"
    "${CONFIG_VALIDATOR_DIR}/pipeline/modifier_config_validator.cpp"
)

set(COMMAND_HANDLER_SOURCES
    "${COMMAND_HANDLER_DIR}/command_dispatcher.cpp"
    "${COMMAND_HANDLER_DIR}/usage_help.cpp"
    "${COMMAND_HANDLER_DIR}/commands/export_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/ingest_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/query_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/simple_command.cpp"
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
    "${CORE_SOURCE_ROOT}/serialization/bills_json_serializer.cpp"
    "${CORE_SOURCE_ROOT}/adapters/serialization/json_bill_serializer.cpp"
    "${CORE_SOURCE_ROOT}/db_insert/parser/bill_json_parser.cpp"
    "${CORE_SOURCE_ROOT}/reports/standard_json/standard_report_assembler.cpp"
    "${CORE_SOURCE_ROOT}/reports/standard_json/standard_report_json_serializer.cpp"
)

set(REPORTS_CORE_SOURCES
    "${CORE_REPORTS_DIR}/core/report_export_service.cpp"
    "${CORE_REPORTS_DIR}/core/report_exporter.cpp"
    "${CORE_REPORTS_DIR}/core/standard_json_latex_renderer.cpp"
    "${CORE_REPORTS_DIR}/core/standard_json_markdown_renderer.cpp"
    "${CORE_REPORTS_DIR}/core/standard_json_typst_renderer.cpp"
    "${CORE_REPORTS_DIR}/monthly_report/monthly_report_generator.cpp"
    "${CORE_REPORTS_DIR}/monthly_report/report_sorter.cpp"
    "${CORE_REPORTS_DIR}/yearly_report/yearly_report_generator.cpp"
)

set(ABI_SOURCES
    "${CORE_ABI_DIR}/bills_core_abi.cpp"
    "${CORE_ABI_DIR}/internal/abi_shared.cpp"
    "${CORE_ABI_DIR}/internal/handlers/validate_handler.cpp"
    "${CORE_ABI_DIR}/internal/handlers/convert_handler.cpp"
    "${CORE_ABI_DIR}/internal/handlers/ingest_import_handler.cpp"
    "${CORE_ABI_DIR}/internal/handlers/query_handler.cpp"
)

set(ADAPTER_SOURCES
    "${ADAPTER_CONFIG_DIR}/json_bill_config_loader.cpp"
    "${ADAPTER_CONFIG_DIR}/json_config_provider.cpp"
    "${ADAPTER_CONFIG_DIR}/json_modifier_config_loader.cpp"
    "${ADAPTER_IO_DIR}/file_bill_content_reader.cpp"
    "${ADAPTER_IO_DIR}/file_bill_file_enumerator.cpp"
    "${ADAPTER_DB_DIR}/sqlite_bill_repository.cpp"
    "${ADAPTER_DB_DIR}/bill_inserter.cpp"
    "${ADAPTER_DB_DIR}/database_manager.cpp"
    "${ADAPTER_DB_DIR}/month_query.cpp"
    "${ADAPTER_DB_DIR}/year_query.cpp"
    "${ADAPTER_DB_DIR}/sqlite_report_db_session.cpp"
    "${ADAPTER_DB_DIR}/sqlite_report_data_gateway.cpp"
    "${WINDOWS_INFRA_DIR}/reports/plugins/common/dynamic_month_report_formatter_provider.cpp"
    "${WINDOWS_INFRA_DIR}/reports/plugins/common/dynamic_yearly_report_formatter_provider.cpp"
)
# reports
set(REPORTS_SOURCES
    "${CLI_REPORTS_DIR}/plugins/month_formatters/base_month_report_formatter.cpp"
    "${CLI_REPORTS_DIR}/plugins/year_formatters/base_yearly_report_formatter.cpp"
)

set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/file_handler.cpp"
)

# --- core 源码：业务规则 + 用例 + 配置校验 ---
set(CORE_SOURCES
    ${CONFIG_VALIDATOR_SOURCES}
    ${CONVERSION_SOURCES}
    ${SERIALIZATION_SOURCES}
    ${APPLICATION_SOURCES}
    ${REPORTS_CORE_SOURCES}
    ${ABI_SOURCES}
)

# --- 平台源码：表现层 + IO 适配 + 导出 ---
set(PLATFORM_SOURCES
    ${COMMAND_HANDLER_SOURCES}
    ${ADAPTER_SOURCES}
    ${REPORTS_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${CONTROLLER_SOURCES}
)


