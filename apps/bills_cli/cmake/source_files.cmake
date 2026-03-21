# ==============================================================================
#  源文件收集
# ==============================================================================
set(CORE_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/bills_core/src")
set(PRESENTATION_DIR "${SOURCE_ROOT}/presentation")
set(CLI_ENTRY_DIR "${PRESENTATION_DIR}/entry")
set(CLI_PARSING_DIR "${PRESENTATION_DIR}/parsing")
set(CLI_OUTPUT_DIR "${PRESENTATION_DIR}/output")
set(CLI_FEATURES_DIR "${PRESENTATION_DIR}/features")
set(CLI_WORKSPACE_FEATURE_DIR "${CLI_FEATURES_DIR}/workspace")
set(CLI_REPORT_FEATURE_DIR "${CLI_FEATURES_DIR}/report")
set(CLI_TEMPLATE_FEATURE_DIR "${CLI_FEATURES_DIR}/template")
set(CLI_CONFIG_FEATURE_DIR "${CLI_FEATURES_DIR}/config")
set(CLI_META_FEATURE_DIR "${CLI_FEATURES_DIR}/meta")

set(CLI_ENTRY_SOURCES
    "${CLI_ENTRY_DIR}/cli_app.cpp"
    "${CLI_ENTRY_DIR}/runtime_context.cpp"
)

set(CLI_PARSING_SOURCES
    "${CLI_PARSING_DIR}/cli_parser.cpp"
)

set(CLI_OUTPUT_SOURCES
    "${CLI_OUTPUT_DIR}/help_text.cpp"
)

set(CLI_FEATURE_SOURCES
    "${CLI_WORKSPACE_FEATURE_DIR}/workspace_handler.cpp"
    "${CLI_REPORT_FEATURE_DIR}/report_handler.cpp"
    "${CLI_TEMPLATE_FEATURE_DIR}/template_handler.cpp"
    "${CLI_CONFIG_FEATURE_DIR}/config_handler.cpp"
    "${CLI_META_FEATURE_DIR}/meta_handler.cpp"
)

set(CLI_APP_SOURCES
    ${CLI_ENTRY_SOURCES}
    ${CLI_PARSING_SOURCES}
    ${CLI_OUTPUT_SOURCES}
    ${CLI_FEATURE_SOURCES}
)
