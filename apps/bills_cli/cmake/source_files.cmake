# ==============================================================================
#  源文件收集
# ==============================================================================
set(CORE_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/core/src")
set(PRESENTATION_DIR "${SOURCE_ROOT}/presentation")
set(CLI_MODULES_DIR "${SOURCE_ROOT}/modules")
set(CLI_ENTRY_DIR "${PRESENTATION_DIR}/entry")
set(CLI_PARSING_DIR "${PRESENTATION_DIR}/parsing")
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
    ${CLI_FEATURE_SOURCES}
)

set(CLI_MODULE_INTERFACE_FILES
    "${CLI_MODULES_DIR}/common_result.cppm"
    "${CLI_MODULES_DIR}/common_utils.cppm"
    "${CLI_MODULES_DIR}/cli_version.cppm"
    "${CLI_MODULES_DIR}/core_version.cppm"
    "${CLI_MODULES_DIR}/nlohmann_json.cppm"
    "${CLI_MODULES_DIR}/io_host_flow_support.cppm"
    "${CLI_MODULES_DIR}/io_factory.cppm"
    "${CLI_MODULES_DIR}/io_year_partition_output_path_builder.cppm"
    "${CLI_MODULES_DIR}/io_report_export_service.cppm"
    "${CLI_MODULES_DIR}/query_service.cppm"
    "${CLI_MODULES_DIR}/report_render_service.cppm"
    "${CLI_MODULES_DIR}/renderer_registry.cppm"
    "${CLI_MODULES_DIR}/record_template_service.cppm"
    "${CLI_MODULES_DIR}/ingest_bill_workflow_service.cppm"
    "${CLI_MODULES_DIR}/cli_app.cppm"
    "${CLI_MODULES_DIR}/runtime_context.cppm"
    "${CLI_MODULES_DIR}/cli_request.cppm"
    "${CLI_MODULES_DIR}/cli_parser.cppm"
    "${CLI_MODULES_DIR}/workspace_handler.cppm"
    "${CLI_MODULES_DIR}/report_handler.cppm"
    "${CLI_MODULES_DIR}/template_handler.cppm"
    "${CLI_MODULES_DIR}/config_handler.cppm"
    "${CLI_MODULES_DIR}/meta_handler.cppm"
)
