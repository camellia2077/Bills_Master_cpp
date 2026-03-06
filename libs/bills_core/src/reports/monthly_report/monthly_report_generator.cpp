// reports/monthly_report/MonthlyReportGenerator.cpp
#include "monthly_report_generator.hpp"

#include <stdexcept>

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
    throw std::runtime_error(
        "Monthly formatter for '" + format_name +
        "' is not available in the current build.");
  }

  return formatter->format_report(data);
}
