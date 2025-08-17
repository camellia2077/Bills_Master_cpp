#include "common/pch.h"
#include "YearMdFormat.h"
#include <sstream>
#include <iomanip>

YearMdFormat::YearMdFormat(const YearMdConfig& config) : config(config) {}

std::string YearMdFormat::get_no_data_message(int year) const {
    std::stringstream ss;
    ss << config.not_found_msg_part1 << year << config.not_found_msg_part2;
    return ss.str();
}

std::string YearMdFormat::generate_header(const YearlyReportData& data) const {
    return ""; // Markdown 没有正式的头部
}

std::string YearMdFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);
    ss << "\n## " << data.year << config.yearly_total_label << ":**"
       << data.grand_total << " " << config.currency_name << "**\n";
    return ss.str();
}

std::string YearMdFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    ss << "\n## " << config.monthly_breakdown_title << "\n";
    return ss.str();
}

std::string YearMdFormat::generate_monthly_item(int year, int month, double total) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);
    ss << " - " << year << config.monthly_item_date_separator
       << std::setfill(config.fill_char) << std::setw(config.month_width) << month
       << config.monthly_item_value_separator << total << " " << config.currency_name << "\n";
    return ss.str();
}

std::string YearMdFormat::generate_footer(const YearlyReportData& data) const {
    return ""; // Markdown 没有正式的尾部
}


// extern "C" 代码块保持不变
extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IYearlyReportFormatter* create_md_year_formatter() {
        return new YearMdFormat();
    }
}