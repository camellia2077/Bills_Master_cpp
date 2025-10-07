// reports/plugins/month_formatters/month_typ/MonthTypFormat.cpp

#include "MonthTypFormat.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath> // For std::abs

MonthTypFormat::MonthTypFormat(const MonthTypConfig& config) : config_(config) {}

std::string MonthTypFormat::escape_typst(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    for (const char c : input) {
        switch (c) {
            case '\\': output += "\\\\"; break;
            case '*':  output += "\\*";  break;
            case '_':  output += "\\_";  break;
            case '#':  output += "\\#";  break;
            default:   output += c;    break;
        }
    }
    return output;
}

std::string MonthTypFormat::get_no_data_message(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << "#set document(title: \"" << data.year << config_.labels.year_suffix << data.month << config_.labels.month_suffix << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    ss << config_.labels.no_data_found << "\n";
    return ss.str();
}

std::string MonthTypFormat::generate_header(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << "#set document(title: \"" << data.year << config_.labels.year_suffix << data.month << config_.labels.month_suffix << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    ss << "= " << data.year << config_.labels.year_suffix << data.month << config_.labels.month_suffix << config_.labels.report_title_suffix << "\n\n";
    return ss.str();
}

std::string MonthTypFormat::generate_summary(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    ss << "*" << config_.labels.income_total << ":* " << config_.currency_symbol << data.total_income << "\n";
    ss << "*" << config_.labels.expense_total << ":* " << config_.currency_symbol << data.total_expense << "\n";
    ss << "*" << config_.labels.balance << ":* " << config_.currency_symbol << data.balance << "\n";
    ss << "*" << config_.labels.remark << ":* " << escape_typst(data.remark) << "\n\n";
    return ss.str();
}

std::string MonthTypFormat::generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);

    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.total_expense != 0) ? (parent_data.parent_total / data.total_expense) * 100.0 : 0.0;

        ss << "== " << escape_typst(parent_name) << "\n\n";
        ss << "*" << config_.labels.category_total << ":* " << config_.currency_symbol << parent_data.parent_total << "\n";
        ss << "*" << config_.labels.percentage_share << ":* " << std::abs(parent_percentage) << config_.percentage_symbol << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total != 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            ss << "=== " << escape_typst(sub_name) << "\n";
            ss << "  *" << config_.labels.sub_category_total << ":* " << config_.currency_symbol << sub_data.sub_total
               << " (" << config_.labels.percentage_share << ": " << std::abs(sub_percentage) << config_.percentage_symbol << ")\n";

            for (const auto& t : sub_data.transactions) {
                ss << "  - " << config_.currency_symbol << t.amount << " " << escape_typst(t.description) << "\n";
            }
            ss << "\n";
        }
    }
    return ss.str();
}

std::string MonthTypFormat::generate_footer(const MonthlyReportData& data) const {
    return "";
}

extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IMonthReportFormatter* create_typ_month_formatter() {
        return new MonthTypFormat();
    }
}
