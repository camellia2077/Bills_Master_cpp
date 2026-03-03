// reports/plugins/month_formatters/BaseMonthReportFormatter.cpp
#include "base_month_report_formatter.hpp"

auto BaseMonthReportFormatter::format_report(
    const MonthlyReportData& data) const -> std::string {
  if (!data.data_found) {
    return get_no_data_message(data);
  }

  // Common logic: sort the data before formatting
  auto sorted_parents = ReportSorter::sort_report_data(data);

  std::stringstream report_stream;
  report_stream << generate_header(data);
  report_stream << generate_summary(data);
  report_stream << generate_body(data, sorted_parents);
  report_stream << generate_footer(data);

  return report_stream.str();
}
