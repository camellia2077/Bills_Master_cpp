// reports/plugins/month_formatters/month_rst/MonthRstFormat.hpp
#ifndef MONTH_RST_FORMAT_HPP
#define MONTH_RST_FORMAT_HPP

#include "reports/plugins/month_formatters/BaseMonthReportFormatter.hpp" // 1. Include the new base class
#include "MonthRstConfig.hpp"

// 2. Inherit from the new base class
class MonthRstFormat : public BaseMonthReportFormatter {
public:
    explicit MonthRstFormat(const MonthRstConfig& config = MonthRstConfig());

protected:
    // 3. Implement the pure virtual functions
    std::string get_no_data_message(const MonthlyReportData& data) const override;
    std::string generate_header(const MonthlyReportData& data) const override;
    std::string generate_summary(const MonthlyReportData& data) const override;
    std::string generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const override;
    std::string generate_footer(const MonthlyReportData& data) const override;

private:
    MonthRstConfig config;
};

#endif // MONTH_RST_FORMAT_HPP
