// reports/plugins/year_formatters/BaseYearlyReportFormatter.cpp
#include "BaseYearlyReportFormatter.hpp"

std::string BaseYearlyReportFormatter::format_report(const YearlyReportData& data) const {
    if (!data.data_found) {
        return get_no_data_message(data.year);
    }

    std::stringstream ss;
    ss << generate_header(data);
    ss << generate_summary(data);
    ss << generate_monthly_breakdown_header();

    for (const auto& pair : data.monthly_summary) {
        ss << generate_monthly_item(data.year, pair.first, pair.second);
    }

    ss << generate_footer(data);

    return ss.str();
}