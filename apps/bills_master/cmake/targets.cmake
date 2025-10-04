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
# CLI 版本
add_executable(bill_master_cli
    "${SOURCE_ROOT}/main_command.cpp"
    # 确保这里使用的是在 source_files.cmake 中定义的、包含了所有源文件的那个变量
    ${SHARED_SOURCES} 
)

# 为 bill_master_cli 应用通用配置
foreach(target bill_master_cli)
    add_dependencies(${target} copy_config)
    target_include_directories(${target} PRIVATE "${SOURCE_ROOT}")
    target_link_libraries(${target} PRIVATE ${COMMON_LINK_LIBRARIES})
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
endforeach()

# ==============================================================================
#  自动复制第三方 DLL
# ==============================================================================
if(WIN32 AND SQLite3_LIBRARIES)
    foreach(target bill_master_cli)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SQLite3_LIBRARIES}"
            $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying SQLite3 DLL to output directory for ${target}"
        )
    endforeach()
endif()