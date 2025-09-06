# ==============================================================================
#  源文件收集
# ==============================================================================
set(REPROCESSING_DIR "${SOURCE_ROOT}/reprocessing")
set(DB_INSERT_DIR    "${SOURCE_ROOT}/db_insert")
set(QUERY_DIR        "${SOURCE_ROOT}/query")
set(FILEHANDLER_DIR  "${SOURCE_ROOT}/file_handler")
set(APPCONTROLLER_DIR "${SOURCE_ROOT}/app_controller")
set(USAGE_HELP_DIR   "${SOURCE_ROOT}/usage_help")

# 1. reprocessing 模块源文件
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
    "${REPROCESSING_DIR}/validator/verifier/BillFormatVerifier.cpp"
)

# 2. db_insert 模块源文件
set(DB_INSERT_SOURCES
    "${DB_INSERT_DIR}/DataProcessor.cpp"

    "${DB_INSERT_DIR}/insertor/BillInserter.cpp"
    "${DB_INSERT_DIR}/insertor/DatabaseManager.cpp"

    "${DB_INSERT_DIR}/parser/BillJsonParser.cpp"
)

# 3. query 模块源文件
set(QUERY_SOURCES
    "${QUERY_DIR}/components/monthly_report/MonthQuery.cpp"
    "${QUERY_DIR}/components/monthly_report/ReportSorter.cpp"
    "${QUERY_DIR}/components/yearly_report/YearQuery.cpp"
    "${QUERY_DIR}/core/BillMetadataReader.cpp"
    "${QUERY_DIR}/core/QueryFacade.cpp"
    "${QUERY_DIR}/plugins/year_formatters/BaseYearlyReportFormatter.cpp"
    
    # --- 新增的源文件 ---
    "${QUERY_DIR}/components/monthly_report/MonthlyReportGenerator.cpp"
    "${QUERY_DIR}/components/yearly_report/YearlyReportGenerator.cpp"
    "${QUERY_DIR}/core/ReportExporter.cpp"
)

# 4. file_handler 模块源文件
set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/FileHandler.cpp"
)

# 5. app_controller 模块源文件
set(APPCONTROLLER_SOURCES
    "${APPCONTROLLER_DIR}/AppController.cpp"
    "${APPCONTROLLER_DIR}/ExportController.cpp"
    "${APPCONTROLLER_DIR}/WorkflowController.cpp"
)

# 6. usage_help 模块源文件
set(USAGE_HELP_SOURCES
    "${USAGE_HELP_DIR}/usage_help.cpp"
)

# 将所有用于主程序的源文件合并到一个列表中
set(SHARED_SOURCES
    ${REPROCESSING_SOURCES}
    ${DB_INSERT_SOURCES}
    ${QUERY_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${APPCONTROLLER_SOURCES}
)