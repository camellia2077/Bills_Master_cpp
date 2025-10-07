// reports/plugins/month_formatters/month_md/MonthMdFormat.cpp

#include "MonthMdFormat.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath> // For std::abs

MonthMdFormat::MonthMdFormat(const MonthMdConfig& config) : config(config) {}

// The main format_report function is gone. We now implement the smaller pieces.

std::string MonthMdFormat::get_no_data_message(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2
       << data.month << config.not_found_msg_part3;
    return ss.str();
}

std::string MonthMdFormat::generate_header(const MonthlyReportData& data) const {
    // Markdown doesn't have a formal header, so we can return an empty string.
    return "";
}

std::string MonthMdFormat::generate_summary(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);

    ss << "\n# " << config.date_label << data.year << std::setfill(config.fill_char) << std::setw(config.month_width) << data.month << std::endl;
    ss << "# " << config.income_label << config.currency_symbol << data.total_income << std::endl;
    ss << "# " << config.expense_label << config.currency_symbol << data.total_expense << std::endl;
    ss << "# " << config.balance_label << config.currency_symbol << data.balance << std::endl;
    ss << "# " << config.remark_label << data.remark << std::endl;

    return ss.str();
}

std::string MonthMdFormat::generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        ss << "\n# " << parent_name << std::endl;
        double parent_percentage = (data.total_expense != 0) ? (parent_data.parent_total / data.total_expense) * 100.0 : 0.0;
        ss << config.parent_total_label << config.currency_symbol << parent_data.parent_total << std::endl;
        ss << config.parent_percentage_label << std::abs(parent_percentage) << config.percentage_symbol << std::endl;

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;

            ss << "\n## " << sub_name << std::endl;
            double sub_percentage = (parent_data.parent_total != 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << config.sub_total_label << config.currency_symbol << sub_data.sub_total
               << config.sub_percentage_label_prefix << std::abs(sub_percentage) << config.sub_percentage_label_suffix << std::endl;

            for (const auto& t : sub_data.transactions) {
                ss << "- " << config.currency_symbol << t.amount << " " << t.description << std::endl;
            }
        }
    }

    return ss.str();
}

std::string MonthMdFormat::generate_footer(const MonthlyReportData& data) const {
    // Markdown doesn't have a formal footer.
    return "";
}

// The plugin export remains the same.
extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IMonthReportFormatter* create_md_month_formatter() {
        return new MonthMdFormat();
    }
}
