// reports/yearly_report/yearly_report_generator.hpp
#ifndef REPORTS_YEARLY_REPORT_YEARLY_REPORT_GENERATOR_H_
#define REPORTS_YEARLY_REPORT_YEARLY_REPORT_GENERATOR_H_

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

#endif  // REPORTS_YEARLY_REPORT_YEARLY_REPORT_GENERATOR_H_
