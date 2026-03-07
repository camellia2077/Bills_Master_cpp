// reports/formatters/year/base_yearly_report_formatter.hpp
#ifndef REPORTS_FORMATTERS_YEAR_BASE_YEARLY_REPORT_FORMATTER_H_
#define REPORTS_FORMATTERS_YEAR_BASE_YEARLY_REPORT_FORMATTER_H_

#include <iomanip>
#include <sstream>
#include <string>

#include "reports/formatters/year/i_yearly_report_formatter.hpp"

class BaseYearlyReportFormatter : public IYearlyReportFormatter {
 public:
  std::string format_report(const YearlyReportData& data) const final;

  virtual ~BaseYearlyReportFormatter() = default;

 protected:
  virtual std::string generate_monthly_item(
      int year, int month, const MonthlySummary& summary) const = 0;
  virtual std::string get_no_data_message(int year) const = 0;
  virtual std::string generate_header(const YearlyReportData& data) const = 0;
  virtual std::string generate_summary(const YearlyReportData& data) const = 0;
  virtual std::string generate_monthly_breakdown_header() const = 0;
  virtual std::string generate_footer(const YearlyReportData& data) const = 0;
};

#endif  // REPORTS_FORMATTERS_YEAR_BASE_YEARLY_REPORT_FORMATTER_H_
