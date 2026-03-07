// reports/formatters/month/rst/month_rst_format.hpp
#ifndef REPORTS_FORMATTERS_MONTH_RST_MONTH_RST_FORMAT_H_
#define REPORTS_FORMATTERS_MONTH_RST_MONTH_RST_FORMAT_H_

#include "month_rst_config.hpp"
#include "reports/formatters/month/base_month_report_formatter.hpp"

// 2. Inherit from the new base class
class MonthRstFormat : public BaseMonthReportFormatter {
 public:
  explicit MonthRstFormat(const MonthRstConfig& config = MonthRstConfig());

 protected:
  // 3. Implement the pure virtual functions
  std::string get_no_data_message(const MonthlyReportData& data) const override;
  std::string generate_header(const MonthlyReportData& data) const override;
  std::string generate_summary(const MonthlyReportData& data) const override;
  std::string generate_body(
      const MonthlyReportData& data,
      const std::vector<std::pair<std::string, ParentCategoryData>>&
          sorted_parents) const override;
  std::string generate_footer(const MonthlyReportData& data) const override;

 private:
  MonthRstConfig config;
};

#endif  // REPORTS_FORMATTERS_MONTH_RST_MONTH_RST_FORMAT_H_
