# ==============================================================================
#  资源复制目标定义
# ==============================================================================
add_custom_target(copy_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SOURCE_ROOT}/config"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config"
    COMMENT "Copying config directory to output"
)

# ==============================================================================
#  可执行文件目标定义
# ==============================================================================
# core 版本
if(BILLS_CORE_BUILD_SHARED)
    set(BILLS_CORE_LIBRARY_TYPE SHARED)
else()
    set(BILLS_CORE_LIBRARY_TYPE STATIC)
endif()

add_library(bills_core ${BILLS_CORE_LIBRARY_TYPE}
    ${CORE_SOURCES}
)
add_library(bill_master_core ALIAS bills_core)
set_target_properties(bills_core PROPERTIES OUTPUT_NAME "bills_core")

if(BILLS_CORE_BUILD_SHARED AND WIN32)
    set_target_properties(bills_core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        PREFIX ""
    )
endif()

# CLI 版本
add_executable(bill_master_cli
    "${CLI_PRESENTATION_DIR}/main_command.cpp"
    ${PLATFORM_SOURCES}
)

# 为构建目标应用通用配置
target_include_directories(bills_core PRIVATE "${CORE_SOURCE_ROOT}")
target_compile_definitions(bills_core PRIVATE BILLS_CORE_ABI_BUILD)
if(BILLS_CORE_BUILD_SHARED)
    target_compile_definitions(bills_core PUBLIC BILLS_CORE_SHARED)
endif()
target_link_libraries(bills_core PRIVATE ${COMMON_LINK_LIBRARIES})
target_compile_options(bills_core PRIVATE ${COMMON_COMPILE_OPTIONS})
if(COMMON_LINK_OPTIONS)
    target_link_options(bills_core PRIVATE ${COMMON_LINK_OPTIONS})
endif()

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

target_link_libraries(bill_master_cli PRIVATE bills_core)
add_dependencies(bill_master_cli copy_config)

if(BILLS_CORE_BUILD_SHARED)
    add_custom_command(
        TARGET bill_master_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bills_core>
        $<TARGET_FILE_DIR:bill_master_cli>
        COMMENT "Copying bills_core runtime library to output directory for bill_master_cli"
    )
endif()
