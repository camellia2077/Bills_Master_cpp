# 调用在根 CMakeLists.txt 中定义的函数来创建插件
create_formatter_plugin("typ_month_formatter"
    "${CMAKE_CURRENT_SOURCE_DIR}/MonthTypFormat.cpp"
    # 因为月度报告需要排序逻辑，所以我们把 ReportSorter.cpp 也作为源文件传入
    "${CMAKE_SOURCE_DIR}/src/reports/components/monthly_report/ReportSorter.cpp"
)