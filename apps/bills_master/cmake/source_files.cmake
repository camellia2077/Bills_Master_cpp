# ==============================================================================
#  源文件收集
# ==============================================================================
set(COMMAND_HANDLER_DIR   "${SOURCE_ROOT}/command_handler")
set(CONVERSION_DIR        "${SOURCE_ROOT}/conversion")
set(DB_INSERT_DIR         "${SOURCE_ROOT}/db_insert")
set(REPORTS_DIR             "${SOURCE_ROOT}/reports")
set(FILEHANDLER_DIR       "${SOURCE_ROOT}/file_handler")
set(APP_CONTROLLER_DIR     "${SOURCE_ROOT}/app_controller")
set(APPLICATION_DIR        "${SOURCE_ROOT}/application")
set(CONFIG_VALIDATOR_DIR     "${SOURCE_ROOT}/config_validator")



# app_controller
set(APP_CONTROLLER_SOURCES
    "${APP_CONTROLLER_DIR}/AppController.cpp"
    "${APP_CONTROLLER_DIR}/export/ExportController.cpp"
    "${APP_CONTROLLER_DIR}/workflow/WorkflowController.cpp"
    "${APP_CONTROLLER_DIR}/workflow/PathBuilder.cpp"
)

set(APPLICATION_SOURCES
    "${APPLICATION_DIR}/use_cases/WorkflowUseCase.cpp"
)

set(CONFIG_VALIDATOR_SOURCES
    "${CONFIG_VALIDATOR_DIR}/facade/ConfigValidator.cpp"
    "${CONFIG_VALIDATOR_DIR}/pipeline/ValidatorConfigValidator.cpp"
    "${CONFIG_VALIDATOR_DIR}/pipeline/ModifierConfigValidator.cpp"
)

set(COMMAND_HANDLER_SOURCES
    "${COMMAND_HANDLER_DIR}/CommandFacade.cpp"
    "${COMMAND_HANDLER_DIR}/usage_help.cpp"
    "${COMMAND_HANDLER_DIR}/commands/ExportCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/IngestCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/QueryCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/SimpleCommand.cpp"
)


set(CONVERSION_SOURCES
    "${CONVERSION_DIR}/Reprocessor.cpp"
    "${CONVERSION_DIR}/convert/BillConverter.cpp"
    "${CONVERSION_DIR}/modifier/processor/BillContentTransformer.cpp"
    "${CONVERSION_DIR}/modifier/processor/BillParser.cpp"
    "${CONVERSION_DIR}/modifier/processor/BillProcessor.cpp"
    "${CONVERSION_DIR}/validator/BillValidator.cpp"
    "${CONVERSION_DIR}/validator/config/BillConfig.cpp"
    "${CONVERSION_DIR}/validator/result/ValidationResult.cpp"
)

set(DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/DataProcessor.cpp"
    "${DB_INSERT_DIR}/insertor/BillInserter.cpp"
    "${DB_INSERT_DIR}/insertor/DatabaseManager.cpp"
)

set(SERIALIZATION_SOURCES
    "${SOURCE_ROOT}/serialization/BillJsonSerializer.cpp"
)

set(ADAPTER_SOURCES
    "${SOURCE_ROOT}/adapters/config/JsonBillConfigLoader.cpp"
    "${SOURCE_ROOT}/adapters/config/JsonConfigProvider.cpp"
    "${SOURCE_ROOT}/adapters/config/JsonModifierConfigLoader.cpp"
    "${SOURCE_ROOT}/adapters/io/FileBillContentReader.cpp"
    "${SOURCE_ROOT}/adapters/io/FileBillFileEnumerator.cpp"
    "${SOURCE_ROOT}/adapters/serialization/JsonBillSerializer.cpp"
    "${SOURCE_ROOT}/adapters/db/SqliteBillRepository.cpp"
)
# reports
set(REPORTS_SOURCES
    "${REPORTS_DIR}/core/BillMetadataReader.cpp"
    "${REPORTS_DIR}/core/QueryFacade.cpp"
    "${REPORTS_DIR}/core/ReportExporter.cpp"

    "${REPORTS_DIR}/monthly_report/MonthlyReportGenerator.cpp"
    "${REPORTS_DIR}/monthly_report/MonthQuery.cpp"
    "${REPORTS_DIR}/monthly_report/ReportSorter.cpp"
    "${REPORTS_DIR}/plugins/month_formatters/BaseMonthReportFormatter.cpp"

    "${REPORTS_DIR}/yearly_report/YearlyReportGenerator.cpp"
    "${REPORTS_DIR}/yearly_report/YearQuery.cpp"

    "${REPORTS_DIR}/plugins/year_formatters/BaseYearlyReportFormatter.cpp"
)

set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/FileHandler.cpp"
)

# --- 将所有源文件聚合到 SHARED_SOURCES ---
set(SHARED_SOURCES
    ${CONFIG_VALIDATOR_SOURCES}
    ${COMMAND_HANDLER_SOURCES}
    ${CONVERSION_SOURCES}
    ${DB_INSERT_SOURCES}
    ${SERIALIZATION_SOURCES}
    ${ADAPTER_SOURCES}
    ${REPORTS_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPLICATION_SOURCES}
    ${APP_CONTROLLER_SOURCES}
)
