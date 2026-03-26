// ports/contracts/reports/formatters/i_yearly_report_formatter.hpp
#ifndef PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_H_
#define PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_H_

#include <string>

#include "ports/contracts/reports/yearly/yearly_report_data.hpp"

class IYearlyReportFormatter {
 public:
  virtual ~IYearlyReportFormatter() = default;

  virtual std::string format_report(const YearlyReportData& data) const = 0;
};

#endif  // PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_H_
