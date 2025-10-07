# reports/plugins/month_formatters/month_typ/CMakeLists.txt
create_formatter_plugin("typ_month_formatter"
    "${CMAKE_CURRENT_SOURCE_DIR}/MonthTypFormat.cpp"
    # Add the new base class implementation
    "${CMAKE_SOURCE_DIR}/src/reports/plugins/month_formatters/BaseMonthReportFormatter.cpp"
    # ReportSorter is now used by the base class, so it still needs to be linked
    "${CMAKE_SOURCE_DIR}/src/reports/monthly_report/ReportSorter.cpp"
)
