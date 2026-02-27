# ==============================================================================
#  源文件收集
# ==============================================================================
set(CORE_SOURCE_ROOT      "${CMAKE_CURRENT_SOURCE_DIR}/../bills_core/src")
set(CONVERSION_DIR        "${CORE_SOURCE_ROOT}/billing/conversion")
set(DB_INSERT_DIR         "${SOURCE_ROOT}/db_insert")
set(REPORTS_DIR           "${SOURCE_ROOT}/reports")
set(CORE_ABI_DIR          "${CORE_SOURCE_ROOT}/abi")
set(APPLICATION_DIR       "${CORE_SOURCE_ROOT}/application")
set(CONFIG_VALIDATOR_DIR  "${CORE_SOURCE_ROOT}/config_validator")
set(CLI_PRESENTATION_DIR  "${SOURCE_ROOT}/platform/windows/presentation/cli")
set(WINDOWS_INFRA_DIR     "${SOURCE_ROOT}/platform/windows/infrastructure")
set(FILEHANDLER_DIR       "${WINDOWS_INFRA_DIR}/file_handler")
set(ADAPTER_CONFIG_DIR    "${WINDOWS_INFRA_DIR}/adapters/config")
set(ADAPTER_IO_DIR        "${WINDOWS_INFRA_DIR}/adapters/io")
set(ADAPTER_DB_DIR        "${WINDOWS_INFRA_DIR}/adapters/db")
set(COMMAND_HANDLER_DIR   "${CLI_PRESENTATION_DIR}/command_handler")
set(CONTROLLERS_DIR       "${CLI_PRESENTATION_DIR}/controllers")



# controllers
set(CONTROLLER_SOURCES
    "${CONTROLLERS_DIR}/AppController.cpp"
    "${CONTROLLERS_DIR}/export/ExportController.cpp"
    "${CONTROLLERS_DIR}/workflow/WorkflowController.cpp"
    "${CONTROLLERS_DIR}/workflow/PathBuilder.cpp"
)

set(APPLICATION_SOURCES
    "${APPLICATION_DIR}/use_cases/workflow_use_case.cpp"
)

set(CONFIG_VALIDATOR_SOURCES
    "${CONFIG_VALIDATOR_DIR}/pipeline/validator_config_validator.cpp"
    "${CONFIG_VALIDATOR_DIR}/pipeline/modifier_config_validator.cpp"
)

set(COMMAND_HANDLER_SOURCES
    "${COMMAND_HANDLER_DIR}/CommandDispatcher.cpp"
    "${COMMAND_HANDLER_DIR}/usage_help.cpp"
    "${COMMAND_HANDLER_DIR}/commands/ExportCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/IngestCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/QueryCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/SimpleCommand.cpp"
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

set(DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/insertor/BillInserter.cpp"
    "${DB_INSERT_DIR}/insertor/DatabaseManager.cpp"
)

set(SERIALIZATION_SOURCES
    "${CORE_SOURCE_ROOT}/serialization/bills_json_serializer.cpp"
    "${CORE_SOURCE_ROOT}/reports/standard_json/standard_report_assembler.cpp"
    "${CORE_SOURCE_ROOT}/reports/standard_json/standard_report_json_serializer.cpp"
)

set(REPORTS_CORE_SOURCES
    "${REPORTS_DIR}/core/ReportExportService.cpp"
    "${REPORTS_DIR}/core/ReportExporter.cpp"
    "${REPORTS_DIR}/core/StandardJsonLatexRenderer.cpp"
    "${REPORTS_DIR}/core/StandardJsonMarkdownRenderer.cpp"
    "${REPORTS_DIR}/core/StandardJsonTypstRenderer.cpp"
    "${REPORTS_DIR}/monthly_report/MonthlyReportGenerator.cpp"
    "${REPORTS_DIR}/monthly_report/ReportSorter.cpp"
    "${REPORTS_DIR}/yearly_report/YearlyReportGenerator.cpp"
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
    "${ADAPTER_CONFIG_DIR}/JsonBillConfigLoader.cpp"
    "${ADAPTER_CONFIG_DIR}/JsonConfigProvider.cpp"
    "${ADAPTER_CONFIG_DIR}/JsonModifierConfigLoader.cpp"
    "${ADAPTER_IO_DIR}/FileBillContentReader.cpp"
    "${ADAPTER_IO_DIR}/FileBillFileEnumerator.cpp"
    "${SOURCE_ROOT}/adapters/serialization/JsonBillSerializer.cpp"
    "${ADAPTER_DB_DIR}/SqliteBillRepository.cpp"
    "${ADAPTER_DB_DIR}/SqliteReportDbSession.cpp"
    "${ADAPTER_DB_DIR}/SqliteReportDataGateway.cpp"
    "${WINDOWS_INFRA_DIR}/reports/plugins/common/DynamicMonthReportFormatterProvider.cpp"
    "${WINDOWS_INFRA_DIR}/reports/plugins/common/DynamicYearlyReportFormatterProvider.cpp"
)
# reports
set(REPORTS_SOURCES
    "${REPORTS_DIR}/plugins/month_formatters/BaseMonthReportFormatter.cpp"
    "${REPORTS_DIR}/plugins/year_formatters/BaseYearlyReportFormatter.cpp"
)

set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/FileHandler.cpp"
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
    ${DB_INSERT_SOURCES}
    ${ADAPTER_SOURCES}
    ${REPORTS_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${CONTROLLER_SOURCES}
)


