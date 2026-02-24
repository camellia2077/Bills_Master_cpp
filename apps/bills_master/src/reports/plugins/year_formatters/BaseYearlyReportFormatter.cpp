// reports/plugins/year_formatters/BaseYearlyReportFormatter.cpp
#include "BaseYearlyReportFormatter.hpp"

auto BaseYearlyReportFormatter::format_report(
    const YearlyReportData& data) const -> std::string {
  if (!data.data_found) {
    return get_no_data_message(data.year);
  }

  std::stringstream report_stream;
  report_stream << generate_header(data);
  report_stream << generate_summary(data);
  report_stream << generate_monthly_breakdown_header();

  for (const auto& pair : data.monthly_summary) {
    report_stream << generate_monthly_item(data.year, pair.first, pair.second);
  }

  report_stream << generate_footer(data);

  return report_stream.str();
}
