// reports/yearly_report/YearlyReportGenerator.cpp
#include "YearlyReportGenerator.hpp"

#include <stdexcept>

namespace {
constexpr const char* kYearFormatterSuffix = "_year_formatter";
}  // namespace

YearlyReportGenerator::YearlyReportGenerator(
    ReportDataGateway* report_data_gateway,
    YearlyReportFormatterProvider* formatter_provider)
    : report_data_gateway_(report_data_gateway),
      formatter_provider_(formatter_provider) {
  if (report_data_gateway_ == nullptr) {
    throw std::invalid_argument("Report data gateway must not be null.");
  }
  if (formatter_provider_ == nullptr) {
    throw std::invalid_argument("Year formatter provider must not be null.");
  }
}

auto YearlyReportGenerator::generate(int year, const std::string& format_name)
    -> std::string {
  YearlyReportData data = report_data_gateway_->ReadYearlyData(year);

  auto formatter = formatter_provider_->CreateFormatter(format_name);

  if (!formatter) {
    std::string expected_plugin_name = format_name + kYearFormatterSuffix;
    throw std::runtime_error(
        "Yearly formatter for '" + format_name +
        "' is not available. Please ensure that the plugin '" +
        expected_plugin_name + "' is available in the plugins directory.");
  }

  return formatter->format_report(data);
}
