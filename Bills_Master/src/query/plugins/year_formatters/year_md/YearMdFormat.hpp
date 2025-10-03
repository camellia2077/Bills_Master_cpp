// query/plugins/year_formatters/year_md/YearMdFormat.hpp
#ifndef YEAR_MD_FORMAT_HPP
#define YEAR_MD_FORMAT_HPP

#include "query/plugins/year_formatters/BaseYearlyReportFormatter.hpp" // 变更了 include
#include "YearMdConfig.hpp"

class YearMdFormat : public BaseYearlyReportFormatter { // 变更了基类
private:
    YearMdConfig config;

public:
    explicit YearMdFormat(const YearMdConfig& config = YearMdConfig());

protected:
    // 实现了来自新基类的纯虚函数
    std::string get_no_data_message(int year) const override;
    std::string generate_header(const YearlyReportData& data) const override;
    std::string generate_summary(const YearlyReportData& data) const override;
    std::string generate_monthly_breakdown_header() const override;
    std::string generate_monthly_item(int year, int month, double total) const override;
    std::string generate_footer(const YearlyReportData& data) const override;
};

#endif // YEAR_MD_FORMAT_HPP