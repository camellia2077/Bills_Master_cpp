// reports/plugins/month_formatters/month_typ/MonthTypFormat.hpp
#ifndef MONTH_TYP_FORMAT_HPP
#define MONTH_TYP_FORMAT_HPP

#include "reports/plugins/month_formatters/BaseMonthReportFormatter.hpp" // 1. Include base
#include "MonthTypConfig.hpp"

// 2. Inherit from base
class MonthTypFormat : public BaseMonthReportFormatter {
public:
    explicit MonthTypFormat(const MonthTypConfig& config = MonthTypConfig{});

protected:
    // 3. Implement virtual functions
    std::string get_no_data_message(const MonthlyReportData& data) const override;
    std::string generate_header(const MonthlyReportData& data) const override;
    std::string generate_summary(const MonthlyReportData& data) const override;
    std::string generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const override;
    std::string generate_footer(const MonthlyReportData& data) const override;

private:
    std::string escape_typst(const std::string& input) const;
    MonthTypConfig config_;
};

#endif // MONTH_TYP_FORMAT_HPP
