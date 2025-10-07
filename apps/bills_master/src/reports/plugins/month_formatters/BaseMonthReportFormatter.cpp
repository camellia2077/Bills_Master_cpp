// reports/plugins/month_formatters/BaseMonthReportFormatter.cpp
#include "BaseMonthReportFormatter.hpp"

std::string BaseMonthReportFormatter::format_report(const MonthlyReportData& data) const {
    if (!data.data_found) {
        return get_no_data_message(data);
    }

    // Common logic: sort the data before formatting
    auto sorted_parents = ReportSorter::sort_report_data(data);

    std::stringstream ss;
    ss << generate_header(data);
    ss << generate_summary(data);
    ss << generate_body(data, sorted_parents);
    ss << generate_footer(data);

    return ss.str();
}
