# 调用在根 CMakeLists.txt 中定义的函数来创建插件
create_formatter_plugin("md_year_formatter"
    # 参数1: 插件自己的源文件
    "${CMAKE_CURRENT_SOURCE_DIR}/YearMdFormat.cpp"

    # 参数2: 它所依赖的基类的源文件
    "${CMAKE_SOURCE_DIR}/src/reports/plugins/year_formatters/BaseYearlyReportFormatter.cpp"
)