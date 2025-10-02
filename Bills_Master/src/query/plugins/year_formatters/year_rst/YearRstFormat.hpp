// query/plugins/year_formatters/year_rst/YearRstFormat.hpp
#ifndef YEAR_RST_FORMAT_HPP
#define YEAR_RST_FORMAT_HPP

#include "query/plugins/year_formatters/BaseYearlyReportFormatter.hpp" // 变更了 include

class YearRstFormat : public BaseYearlyReportFormatter { // 变更了基类
protected:
    std::string get_no_data_message(int year) const override;
    std::string generate_header(const YearlyReportData& data) const override;
    std::string generate_summary(const YearlyReportData& data) const override;
    std::string generate_monthly_breakdown_header() const override;
    std::string generate_monthly_item(int year, int month, double total) const override;
    std::string generate_footer(const YearlyReportData& data) const override;
};

#endif // YEAR_RST_FORMAT_HPP