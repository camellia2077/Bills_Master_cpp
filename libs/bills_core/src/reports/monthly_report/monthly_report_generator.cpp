// reports/monthly_report/MonthlyReportGenerator.cpp
#include "monthly_report_generator.hpp"

#include <stdexcept>

namespace {
constexpr const char* kMonthFormatterSuffix = "_month_formatter";
}  // namespace

MonthlyReportGenerator::MonthlyReportGenerator(
    ReportDataGateway* report_data_gateway,
    MonthReportFormatterProvider* formatter_provider)
    : report_data_gateway_(report_data_gateway),
      formatter_provider_(formatter_provider) {
  if (report_data_gateway_ == nullptr) {
    throw std::invalid_argument("Report data gateway must not be null.");
  }
  if (formatter_provider_ == nullptr) {
    throw std::invalid_argument("Month formatter provider must not be null.");
  }
}

auto MonthlyReportGenerator::generate(int year, int month,
                                      const std::string& format_name)
    -> std::string {
  MonthlyReportData data = report_data_gateway_->ReadMonthlyData(year, month);
  auto formatter = formatter_provider_->CreateFormatter(format_name);

  if (!formatter) {
    std::string expected_plugin_name = format_name + kMonthFormatterSuffix;
    throw std::runtime_error(
        "Monthly formatter for '" + format_name +
        "' is not available. Please ensure that the plugin '" +
        expected_plugin_name + "' is available in the plugins directory.");
  }

  return formatter->format_report(data);
}
