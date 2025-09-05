# ==============================================================================
#  可执行文件目标定义
# ==============================================================================
# GUI/TUI 版本
add_executable(bill_matser_app
    "${SOURCE_ROOT}/main.cpp"
    ${SHARED_SOURCES}
)

# CLI 版本
add_executable(bill_master_cli
    "${SOURCE_ROOT}/main_command.cpp"
    ${SHARED_SOURCES}
    ${USAGE_HELP_SOURCES}
)

# 为所有可执行目标应用通用配置
foreach(target bill_matser_app bill_master_cli)
    # 添加包含目录
    target_include_directories(${target} PRIVATE "${SOURCE_ROOT}")
    
    # 链接外部库
    target_link_libraries(${target} PRIVATE ${COMMON_LINK_LIBRARIES})
    
    # 设置编译选项
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
endforeach()

# ==============================================================================
#  自动复制第三方 DLL (例如 sqlite3.dll)
# ==============================================================================
if(WIN32 AND SQLite3_LIBRARIES)
    foreach(target bill_matser_app bill_master_cli)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SQLite3_LIBRARIES}"
            $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying SQLite3 DLL to output directory for ${target}"
        )
    endforeach()
endif()