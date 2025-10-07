// reports/plugins/month_formatters/month_tex/MonthTexFormat.hpp
#ifndef MONTH_TEX_FORMAT_HPP
#define MONTH_TEX_FORMAT_HPP

#include "reports/plugins/month_formatters/BaseMonthReportFormatter.hpp" // 1. Include base class
#include "MonthTexReport.hpp"

// 2. Inherit from base class
class MonthTexFormat : public BaseMonthReportFormatter {
public:
    explicit MonthTexFormat(const MonthTexReport& config = MonthTexReport{});

protected:
    // 3. Implement virtual functions
    std::string get_no_data_message(const MonthlyReportData& data) const override;
    std::string generate_header(const MonthlyReportData& data) const override;
    std::string generate_summary(const MonthlyReportData& data) const override;
    std::string generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const override;
    std::string generate_footer(const MonthlyReportData& data) const override;

private:
    std::string escape_latex(const std::string& input) const;
    MonthTexReport m_config;
};

#endif // MONTH_TEX_FORMAT_HPP
