// reports/formatters/year/md/year_md_format.hpp
#ifndef REPORTS_FORMATTERS_YEAR_MD_YEAR_MD_FORMAT_H_
#define REPORTS_FORMATTERS_YEAR_MD_YEAR_MD_FORMAT_H_

#include "year_md_config.hpp"
#include "reports/formatters/year/base_yearly_report_formatter.hpp"

class YearMdFormat : public BaseYearlyReportFormatter {
 public:
  explicit YearMdFormat(YearMdConfig config = YearMdConfig());

 protected:
  auto get_no_data_message(int year) const -> std::string override;
  auto generate_header(const YearlyReportData& data) const
      -> std::string override;
  auto generate_summary(const YearlyReportData& data) const
      -> std::string override;
  auto generate_monthly_breakdown_header() const -> std::string override;
  auto generate_monthly_item(int year, int month,
                             const MonthlySummary& summary) const
      -> std::string override;
  auto generate_footer(const YearlyReportData& data) const
      -> std::string override;

 private:
  YearMdConfig config;
};

#endif  // REPORTS_FORMATTERS_YEAR_MD_YEAR_MD_FORMAT_H_
