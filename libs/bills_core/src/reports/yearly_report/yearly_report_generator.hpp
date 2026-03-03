// reports/yearly_report/YearlyReportGenerator.hpp
#ifndef YEARLY_REPORT_GENERATOR_HPP
#define YEARLY_REPORT_GENERATOR_HPP

#include <string>

#include "ports/report_data_gateway.hpp"
#include "ports/yearly_report_formatter_provider.hpp"

class YearlyReportGenerator {
 public:
  explicit YearlyReportGenerator(ReportDataGateway* report_data_gateway,
                                 YearlyReportFormatterProvider* formatter_provider);

  std::string generate(int year, const std::string& format_name);

 private:
  ReportDataGateway* report_data_gateway_;
  YearlyReportFormatterProvider* formatter_provider_;
};

#endif  // YEARLY_REPORT_GENERATOR_HPP
