// reports/plugins/month_formatters/month_md/MonthMdFormat.hpp
#ifndef MONTH_MD_FORMAT_HPP
#define MONTH_MD_FORMAT_HPP

#include "reports/plugins/month_formatters/BaseMonthReportFormatter.hpp" // 1. Include the new base class
#include "MonthMdConfig.hpp"

// 2. Inherit from the new base class
class MonthMdFormat : public BaseMonthReportFormatter {
public:
    explicit MonthMdFormat(const MonthMdConfig& config = MonthMdConfig());

protected:
    // 3. Implement the pure virtual functions from the base class
    std::string get_no_data_message(const MonthlyReportData& data) const override;
    std::string generate_header(const MonthlyReportData& data) const override;
    std::string generate_summary(const MonthlyReportData& data) const override;
    std::string generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const override;
    std::string generate_footer(const MonthlyReportData& data) const override;

private:
    MonthMdConfig config; // Keep the config object
};

#endif // MONTH_MD_FORMAT_HPP
