// reports/monthly_report/monthly_report_generator.hpp
#ifndef REPORTS_MONTHLY_REPORT_MONTHLY_REPORT_GENERATOR_H_
#define REPORTS_MONTHLY_REPORT_MONTHLY_REPORT_GENERATOR_H_

#include <string>

#include "ports/month_report_formatter_provider.hpp"
#include "ports/report_data_gateway.hpp"

class MonthlyReportGenerator {
 public:
  explicit MonthlyReportGenerator(ReportDataGateway* report_data_gateway,
                                  MonthReportFormatterProvider* formatter_provider);

  std::string generate(int year, int month, const std::string& format_name);

 private:
  ReportDataGateway* report_data_gateway_;
  MonthReportFormatterProvider* formatter_provider_;
};

#endif  // REPORTS_MONTHLY_REPORT_MONTHLY_REPORT_GENERATOR_H_
