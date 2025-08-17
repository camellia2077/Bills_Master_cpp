# 调用在根 CMakeLists.txt 中定义的函数来创建插件 
create_formatter_plugin("md_month_formatter"
    # 最佳实践：为本地文件也使用绝对路径，使其更加明确
    "${CMAKE_CURRENT_SOURCE_DIR}/MonthMdFormat.cpp"
    # 因为月度报告需要排序逻辑，所以我们把 ReportSorter.cpp 也作为源文件传入 
    # 修正：在下面的路径中加入了 "src/"
    "${CMAKE_SOURCE_DIR}/src/query/components/monthly_report/ReportSorter.cpp"
)