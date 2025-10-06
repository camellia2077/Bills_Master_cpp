// reports/plugins/year_formatters/year_tex/YearTexFormat.hpp
#ifndef YEAR_TEX_FORMAT_HPP
#define YEAR_TEX_FORMAT_HPP

#include "reports/plugins/year_formatters/BaseYearlyReportFormatter.hpp" 
#include "YearlyTexConfig.hpp"

class YearTexFormat : public BaseYearlyReportFormatter { 
public:
    explicit YearTexFormat(const YearlyTexConfig& config = YearlyTexConfig{});

private:
    std::string escape_latex(const std::string& input) const;
    YearlyTexConfig m_config;

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

#endif // YEAR_TEX_FORMAT_HPP