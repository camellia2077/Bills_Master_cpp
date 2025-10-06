// reports/plugins/year_formatters/year_md/YearMdFormat.hpp
#ifndef YEAR_MD_FORMAT_HPP
#define YEAR_MD_FORMAT_HPP

#include "reports/plugins/year_formatters/BaseYearlyReportFormatter.hpp"
#include "YearMdConfig.hpp"

class YearMdFormat : public BaseYearlyReportFormatter {
private:
    YearMdConfig config;

public:
    explicit YearMdFormat(const YearMdConfig& config = YearMdConfig());

protected:
    std::string get_no_data_message(int year) const override;
    std::string generate_header(const YearlyReportData& data) const override;
    std::string generate_summary(const YearlyReportData& data) const override;
    std::string generate_monthly_breakdown_header() const override;
    // --- 【核心修改】: 更新函数签名 ---
    std::string generate_monthly_item(int year, int month, const MonthlySummary& summary) const override;
    // --- 修改结束 ---
    std::string generate_footer(const YearlyReportData& data) const override;
};

#endif // YEAR_MD_FORMAT_HPP