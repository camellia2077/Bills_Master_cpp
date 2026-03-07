// reports/formatters/month/md/month_md_format.hpp
#ifndef REPORTS_FORMATTERS_MONTH_MD_MONTH_MD_FORMAT_H_
#define REPORTS_FORMATTERS_MONTH_MD_MONTH_MD_FORMAT_H_

#include "month_md_config.hpp"
#include "reports/formatters/month/base_month_report_formatter.hpp"

class MonthMdFormat : public BaseMonthReportFormatter {
 public:
  explicit MonthMdFormat(MonthMdConfig config = MonthMdConfig());

 protected:
  auto get_no_data_message(const MonthlyReportData& data) const
      -> std::string override;
  auto generate_header(const MonthlyReportData& data) const
      -> std::string override;
  auto generate_summary(const MonthlyReportData& data) const
      -> std::string override;
  auto generate_body(
      const MonthlyReportData& data,
      const std::vector<std::pair<std::string, ParentCategoryData>>&
          sorted_parents) const -> std::string override;
  auto generate_footer(const MonthlyReportData& data) const
      -> std::string override;

 private:
  MonthMdConfig config;
};

#endif  // REPORTS_FORMATTERS_MONTH_MD_MONTH_MD_FORMAT_H_
