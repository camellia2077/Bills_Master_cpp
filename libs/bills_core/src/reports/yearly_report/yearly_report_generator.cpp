// reports/yearly_report/YearlyReportGenerator.cpp
#include "yearly_report_generator.hpp"

#include <stdexcept>

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
    throw std::runtime_error(
        "Yearly formatter for '" + format_name +
        "' is not available in the current build.");
  }

  return formatter->format_report(data);
}
