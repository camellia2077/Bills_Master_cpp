# ==============================================================================
#  插件构建函数
# ==============================================================================
function(create_formatter_plugin name)
    set(sources ${ARGN})
    add_library(${name} SHARED ${sources})
    
    target_include_directories(${name} PRIVATE "${SOURCE_ROOT}")
    
    target_compile_options(${name} PRIVATE
        # 为Release模式下的插件添加优化和瘦身选项
        $<$<CONFIG:Release>:-Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections>
        ${FORCE_INCLUDE_PCH} # 应用自动包含 PCH 的选项
    )
    
    target_link_options(${name} PRIVATE
        $<$<CONFIG:Release>:-Wl,--gc-sections -s>
    )
    
    # 移除库文件的前缀 (例如，在Linux上从 libxxx.so 变为 xxx.so)
    set_target_properties(${name} PROPERTIES PREFIX "")
    
    message(STATUS "Configured formatter plugin '${name}' via function.")
endfunction()

# ==============================================================================
#  加载插件模块
# ==============================================================================
message(STATUS "Loading report formatter plugins...")
# --- 月度报告插件 ---
add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_md)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_rst)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_tex)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_typ)

# --- 年度报告插件 ---
add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_md)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_tex)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_rst)
add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_typ)

message(STATUS "All plugins configured.")