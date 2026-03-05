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
# core 版本（始终提供 static 供 CLI 链接）
add_library(bills_core STATIC ${CORE_SOURCES})
add_library(bill_master_core ALIAS bills_core)
set_target_properties(bills_core PROPERTIES OUTPUT_NAME "bills_core")

target_include_directories(bills_core PRIVATE
    "${CORE_SOURCE_ROOT}"
    "${SOURCE_ROOT}"
)
target_compile_definitions(bills_core PRIVATE BILLS_CORE_ABI_BUILD)
target_link_libraries(bills_core PRIVATE
    ${COMMON_LINK_LIBRARIES}
    sqlite3_amalgamation
)
target_compile_options(bills_core PRIVATE ${COMMON_COMPILE_OPTIONS})
if(COMMON_LINK_OPTIONS)
    target_link_options(bills_core PRIVATE ${COMMON_LINK_OPTIONS})
endif()

# 运行时 core.dll（用于部署与测试环境复制）
if(BILLS_CORE_BUILD_SHARED)
    add_library(bills_core_runtime SHARED ${CORE_SOURCES})
    set_target_properties(bills_core_runtime PROPERTIES OUTPUT_NAME "bills_core")
    if(WIN32)
        set_target_properties(bills_core_runtime PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            PREFIX ""
        )
    endif()
    target_include_directories(bills_core_runtime PRIVATE
        "${CORE_SOURCE_ROOT}"
        "${SOURCE_ROOT}"
    )
    target_compile_definitions(bills_core_runtime PRIVATE
        BILLS_CORE_ABI_BUILD
        BILLS_CORE_SHARED
    )
    target_link_libraries(bills_core_runtime PRIVATE
        ${COMMON_LINK_LIBRARIES}
        sqlite3_amalgamation
    )
    target_compile_options(bills_core_runtime PRIVATE ${COMMON_COMPILE_OPTIONS})
    if(COMMON_LINK_OPTIONS)
        target_link_options(bills_core_runtime PRIVATE ${COMMON_LINK_OPTIONS})
    endif()
endif()

# CLI 版本
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

if(BILLS_CORE_BUILD_SHARED)
    add_dependencies(bill_master_cli bills_core_runtime)
    add_custom_command(
        TARGET bill_master_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bills_core_runtime>
        $<TARGET_FILE_DIR:bill_master_cli>
        COMMENT "Copying bills_core runtime library to output directory for bill_master_cli"
    )
endif()
