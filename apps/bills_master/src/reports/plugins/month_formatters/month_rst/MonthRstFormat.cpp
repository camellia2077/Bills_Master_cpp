// reports/plugins/month_formatters/month_rst/MonthRstFormat.cpp

#include "MonthRstFormat.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath> // For std::abs

MonthRstFormat::MonthRstFormat(const MonthRstConfig& config) : config(config) {}

std::string MonthRstFormat::get_no_data_message(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2
       << data.month << config.not_found_msg_part3;
    return ss.str();
}

std::string MonthRstFormat::generate_header(const MonthlyReportData& data) const {
    std::stringstream ss;
    std::string title = std::to_string(data.year) + "年" + std::to_string(data.month) + config.report_title_suffix;
    ss << title << "\n";
    ss << std::string(title.length() * 2, config.title_char) << "\n\n";
    return ss.str();
}

std::string MonthRstFormat::generate_summary(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);
    ss << config.income_prefix << config.currency_symbol << data.total_income << "\n";
    ss << config.expense_prefix << config.currency_symbol << data.total_expense << "\n";
    ss << config.balance_prefix << config.currency_symbol << data.balance << "\n";
    ss << config.remark_prefix << data.remark << "\n\n";
    return ss.str();
}

std::string MonthRstFormat::generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        ss << parent_name << "\n";
        ss << std::string(parent_name.length() * 2, config.parent_char) << "\n";

        double parent_percentage = (data.total_expense != 0) ? (parent_data.parent_total / data.total_expense) * 100.0 : 0.0;
        ss << config.parent_total_label << config.currency_symbol << parent_data.parent_total << "\n";
        ss << config.parent_percentage_label << std::abs(parent_percentage) << config.percentage_symbol << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;

            ss << sub_name << "\n";
            ss << std::string(sub_name.length() * 2, config.sub_char) << "\n";

            double sub_percentage = (parent_data.parent_total != 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << config.sub_total_label << config.currency_symbol << sub_data.sub_total
               << config.sub_percentage_label_prefix << std::abs(sub_percentage) << config.sub_percentage_label_suffix << "\n\n";

            for (const auto& t : sub_data.transactions) {
                ss << config.transaction_char << " " << config.currency_symbol << t.amount << " " << t.description << "\n";
            }
            ss << "\n";
        }
    }

    return ss.str();
}

std::string MonthRstFormat::generate_footer(const MonthlyReportData& data) const {
    return "";
}

extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IMonthReportFormatter* create_rst_month_formatter() {
        return new MonthRstFormat();
    }
}
