# 调用在根 CMakeLists.txt 中定义的函数来创建插件
create_formatter_plugin("tex_month_formatter"
    "MonthTexFormat.cpp"
    # 因为月度报告需要排序逻辑，所以我们把 ReportSorter.cpp 也作为源文件传入
    "${CMAKE_SOURCE_DIR}/query/components/monthly_report/ReportSorter.cpp"
)