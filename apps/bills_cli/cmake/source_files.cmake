# ==============================================================================
#  源文件收集
# ==============================================================================
set(CORE_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/bills_core/src")
set(CLI_PRESENTATION_DIR "${SOURCE_ROOT}/windows/presentation/cli")
set(WINDOWS_INFRA_DIR "${SOURCE_ROOT}/windows/infrastructure")
set(FILEHANDLER_DIR "${WINDOWS_INFRA_DIR}/file_handler")
set(COMMAND_HANDLER_DIR "${CLI_PRESENTATION_DIR}/command_handler")
set(CONTROLLERS_DIR "${CLI_PRESENTATION_DIR}/controllers")

set(CONTROLLER_SOURCES
    "${CONTROLLERS_DIR}/app_controller.cpp"
    "${CONTROLLERS_DIR}/export/export_controller.cpp"
    "${CONTROLLERS_DIR}/workflow/workflow_controller.cpp"
    "${CONTROLLERS_DIR}/workflow/path_builder.cpp"
)

set(COMMAND_HANDLER_SOURCES
    "${COMMAND_HANDLER_DIR}/command_dispatcher.cpp"
    "${COMMAND_HANDLER_DIR}/usage_help.cpp"
    "${COMMAND_HANDLER_DIR}/commands/config_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/export_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/ingest_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/query_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/record_command.cpp"
    "${COMMAND_HANDLER_DIR}/commands/simple_command.cpp"
)

set(FILEHANDLER_SOURCES
    "${FILEHANDLER_DIR}/file_handler.cpp"
)

set(PLATFORM_SOURCES
    ${COMMAND_HANDLER_SOURCES}
    ${FILEHANDLER_SOURCES}
    ${CONTROLLER_SOURCES}
)
