# ==============================================================================
#  源文件收集
# ==============================================================================
set(COMMAND_HANDLER_DIR   "${SOURCE_ROOT}/command_handler")
set(REPROCESSING_DIR      "${SOURCE_ROOT}/reprocessing")
set(DB_INSERT_DIR         "${SOURCE_ROOT}/db_insert")
set(REPORTS_DIR             "${SOURCE_ROOT}/reports")
set(FILEHANDLER_DIR       "${SOURCE_ROOT}/file_handler")
set(APP_CONTROLLER_DIR     "${SOURCE_ROOT}/app_controller")
set(CONFIG_VALIDATOR_DIR     "${SOURCE_ROOT}/config_validator")



# app_controller
set(APP_CONTROLLER_SOURCES
    "${APP_CONTROLLER_DIR}/AppController.cpp"
    "${APP_CONTROLLER_DIR}/ConfigLoader.cpp"
    "${APP_CONTROLLER_DIR}/export/ExportController.cpp"
    "${APP_CONTROLLER_DIR}/workflow/WorkflowController.cpp"
    "${APP_CONTROLLER_DIR}/workflow/PathBuilder.cpp"
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
    "${COMMAND_HANDLER_DIR}/commands/QueryCommand.cpp"
    "${COMMAND_HANDLER_DIR}/commands/SimpleCommand.cpp"
)


set(REPROCESSING_SOURCES
    "${REPROCESSING_DIR}/Reprocessor.cpp"
    "${REPROCESSING_DIR}/modifier/BillModifier.cpp"
    "${REPROCESSING_DIR}/modifier/config_loader/ConfigLoader.cpp"
    "${REPROCESSING_DIR}/modifier/processor/BillContentTransformer.cpp"
    "${REPROCESSING_DIR}/modifier/processor/BillParser.cpp"
    "${REPROCESSING_DIR}/modifier/processor/BillProcessor.cpp"
    "${REPROCESSING_DIR}/modifier/raw_format/BillJsonFormatter.cpp"
    "${REPROCESSING_DIR}/validator/BillValidator.cpp"
    "${REPROCESSING_DIR}/validator/config/BillConfig.cpp"
    "${REPROCESSING_DIR}/validator/result/ValidationResult.cpp"
)

set(DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/DataProcessor.cpp"
    "${DB_INSERT_DIR}/insertor/BillInserter.cpp"
    "${DB_INSERT_DIR}/insertor/DatabaseManager.cpp"
    "${DB_INSERT_DIR}/parser/BillJsonParser.cpp"
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
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${REPORTS_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APP_CONTROLLER_SOURCES}
)