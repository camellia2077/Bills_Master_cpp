# ==============================================================================
#  插件构建函数
# ==============================================================================
function(create_formatter_plugin name)
    set(sources ${ARGN})
    add_library(${name} SHARED ${sources})
    
    target_include_directories(${name} PRIVATE "${SOURCE_ROOT}" "${CORE_SOURCE_ROOT}")
    
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

# --- Markdown ---
if(ENABLE_FMT_MD)
    message(STATUS "Markdown formatters are embedded in bill_master_cli; skip md shared plugins.")
else()
    message(STATUS "Skipped Markdown formatter setup (ENABLE_FMT_MD=OFF)")
endif()

# --- reStructuredText ---
if(ENABLE_FMT_RST)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_rst)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_rst)
else()
    message(STATUS "Skipped formatter plugins for format 'rst' (ENABLE_FMT_RST=OFF)")
endif()

# --- LaTeX ---
if(ENABLE_FMT_TEX)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_tex)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_tex)
else()
    message(STATUS "Skipped formatter plugins for format 'tex' (ENABLE_FMT_TEX=OFF)")
endif()

# --- Typst ---
if(ENABLE_FMT_TYP)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/month_formatters/month_typ)
    add_subdirectory(${SOURCE_ROOT}/reports/plugins/year_formatters/year_typ)
else()
    message(STATUS "Skipped formatter plugins for format 'typ' (ENABLE_FMT_TYP=OFF)")
endif()

message(STATUS "All plugins configured.")
