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
add_executable(bill_master_cli
    "${CLI_PRESENTATION_DIR}/main_command.cpp"
    ${PLATFORM_SOURCES}
)

target_include_directories(bill_master_cli PRIVATE
    "${SOURCE_ROOT}"
    "${CORE_SOURCE_ROOT}"
    "${CLI_PRESENTATION_DIR}"
)
target_link_libraries(bill_master_cli PRIVATE
    ${COMMON_LINK_LIBRARIES}
    ${CLI_LINK_LIBRARIES}
)
target_compile_options(bill_master_cli PRIVATE ${COMMON_COMPILE_OPTIONS})
if(COMMON_LINK_OPTIONS)
    target_link_options(bill_master_cli PRIVATE ${COMMON_LINK_OPTIONS})
endif()

target_link_libraries(bill_master_cli PRIVATE
    bills_core
    bills_io
)
add_dependencies(bill_master_cli copy_config)
