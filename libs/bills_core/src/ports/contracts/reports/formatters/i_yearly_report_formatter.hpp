#ifndef PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_HPP
#define PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_HPP

#include <string>

#include "ports/contracts/reports/yearly/yearly_report_data.hpp"

class IYearlyReportFormatter {
 public:
  virtual ~IYearlyReportFormatter() = default;

  virtual std::string format_report(const YearlyReportData& data) const = 0;
};

#endif  // PORTS_CONTRACTS_REPORTS_FORMATTERS_I_YEARLY_REPORT_FORMATTER_HPP
