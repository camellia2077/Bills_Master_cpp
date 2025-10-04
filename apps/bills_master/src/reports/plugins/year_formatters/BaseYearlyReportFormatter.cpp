// reports/plugins/year_formatters/BaseYearlyReportFormatter.cpp
#include "BaseYearlyReportFormatter.hpp"
// 先检查数据是否存在，然后依次调用各个虚函数来生成报告的头部、摘要和每月明细等部分
std::string BaseYearlyReportFormatter::format_report(const YearlyReportData& data) const {
    if (!data.data_found) {
        return get_no_data_message(data.year);
    }

    std::stringstream ss;
    ss << generate_header(data);
    ss << generate_summary(data);
    ss << generate_monthly_breakdown_header();

    for (const auto& pair : data.monthly_totals) {
        ss << generate_monthly_item(data.year, pair.first, pair.second);
    }

    ss << generate_footer(data);

    return ss.str();
}