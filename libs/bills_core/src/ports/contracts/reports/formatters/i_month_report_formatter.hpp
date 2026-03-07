// ports/contracts/reports/formatters/i_month_report_formatter.hpp
#ifndef PORTS_CONTRACTS_REPORTS_FORMATTERS_I_MONTH_REPORT_FORMATTER_H_
#define PORTS_CONTRACTS_REPORTS_FORMATTERS_I_MONTH_REPORT_FORMATTER_H_

#include <string>

#include "ports/contracts/reports/monthly/monthly_report_data.hpp"

class IMonthReportFormatter {
 public:
  virtual ~IMonthReportFormatter() = default;

  virtual std::string format_report(const MonthlyReportData& data) const = 0;
};

#endif  // PORTS_CONTRACTS_REPORTS_FORMATTERS_I_MONTH_REPORT_FORMATTER_H_
