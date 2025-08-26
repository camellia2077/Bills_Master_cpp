
#include "YearRstFormat.hpp"
#include <sstream>
#include <iomanip>

std::string YearRstFormat::get_no_data_message(int year) const {
    return "未找到 " + std::to_string(year) + " 年的任何数据。\n";
}

std::string YearRstFormat::generate_header(const YearlyReportData& data) const {
    std::stringstream ss;
    std::string title = std::to_string(data.year) + "年 消费总览";
    ss << title << "\n";
    ss << std::string(title.length() * 2, '=') << "\n\n";
    return ss.str();
}

std::string YearRstFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "**年度总支出:** CNY" << data.grand_total << "\n\n";
    return ss.str();
}

std::string YearRstFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    std::string subtitle = "每月支出";
    ss << subtitle << "\n";
    ss << std::string(subtitle.length() * 2, '-') << "\n\n";
    return ss.str();
}

std::string YearRstFormat::generate_monthly_item(int year, int month, double total) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "* " << year << "-" << std::setfill('0') << std::setw(2) << month
       << ": CNY" << total << "\n";
    return ss.str();
}

std::string YearRstFormat::generate_footer(const YearlyReportData& data) const {
    return "";
}


// extern "C" 代码块保持不变
extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IYearlyReportFormatter* create_rst_year_formatter() {
        return new YearRstFormat();
    }
}