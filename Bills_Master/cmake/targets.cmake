# ==============================================================================
#  资源复制目标定义 (新添加的部分)
# ==============================================================================
# 创建一个自定义目标，用于将 config 文件夹复制到输出目录
# 使用 add_custom_target 可以确保每次构建时都会检查目录
add_custom_target(copy_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SOURCE_ROOT}/config" # 源目录: src/config
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config" # 目标目录: build/bin/config
    COMMENT "Copying config directory to output"
)

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
    # 添加依赖项，确保在构建可执行文件之前先复制 config 文件夹 (新添加的部分)
    add_dependencies(${target} copy_config)

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