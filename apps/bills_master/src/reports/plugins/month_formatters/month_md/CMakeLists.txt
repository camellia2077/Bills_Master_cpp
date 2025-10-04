# 调用在根 CMakeLists.txt 中定义的函数来创建插件 
create_formatter_plugin("md_month_formatter"
    # 最佳实践：为本地文件也使用绝对路径，使其更加明确
    "${CMAKE_CURRENT_SOURCE_DIR}/MonthMdFormat.cpp"
    "${CMAKE_SOURCE_DIR}/src/reports/components/monthly_report/ReportSorter.cpp"
)