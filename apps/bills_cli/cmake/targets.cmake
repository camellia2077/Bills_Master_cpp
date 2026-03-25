# ==============================================================================
#  资源复制目标定义
# ==============================================================================
add_custom_target(copy_config
    COMMAND ${CMAKE_COMMAND} -E remove_directory
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/../../config"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config"
    COMMENT "Copying config directory to output"
)

# ==============================================================================
#  可执行文件目标定义
# ==============================================================================
add_executable(bills_tracer_cli
    "${CLI_ENTRY_DIR}/main_command.cpp"
    ${CLI_APP_SOURCES}
)

if(BILLS_CLI_ENABLE_MODULES)
    target_sources(bills_tracer_cli PRIVATE
        FILE_SET CXX_MODULES FILES
            ${CLI_MODULE_INTERFACE_FILES}
    )
    target_compile_definitions(bills_tracer_cli PRIVATE
        BILLS_CLI_MODULES_ENABLED=1
    )
endif()

target_include_directories(bills_tracer_cli PRIVATE
    "${SOURCE_ROOT}"
    "${CORE_SOURCE_ROOT}"
)
target_link_libraries(bills_tracer_cli PRIVATE
    ${COMMON_LINK_LIBRARIES}
    ${CLI_LINK_LIBRARIES}
)
target_compile_options(bills_tracer_cli PRIVATE ${COMMON_COMPILE_OPTIONS})
if(COMMON_LINK_OPTIONS)
    target_link_options(bills_tracer_cli PRIVATE ${COMMON_LINK_OPTIONS})
endif()

target_link_libraries(bills_tracer_cli PRIVATE
    bills_core
    bills_io
)
add_dependencies(bills_tracer_cli copy_config)
