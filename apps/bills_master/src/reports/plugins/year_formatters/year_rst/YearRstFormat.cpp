// reports/plugins/year_formatters/year_rst/YearRstFormat.cpp

#include "YearRstFormat.hpp"
#include <sstream>
#include <iomanip>

// ... get_no_data_message 和 generate_header 函数保持不变 ...
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

// --- 【核心修改】: 更新摘要部分 ---
std::string YearRstFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "**年总收入:** CNY" << data.total_income << "\n";
    ss << "**年总支出:** CNY" << data.total_expense << "\n";
    ss << "**年终结余:** CNY" << data.balance << "\n\n";
    return ss.str();
}
// --- 修改结束 ---

// ... generate_monthly_breakdown_header 函数保持不变 ...
std::string YearRstFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    std::string subtitle = "每月明细";
    ss << subtitle << "\n";
    ss << std::string(subtitle.length() * 2, '-') << "\n\n";
    return ss.str();
}


// --- 【核心修改】: 更新月度项目 ---
std::string YearRstFormat::generate_monthly_item(int year, int month, const MonthlySummary& summary) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "* " << year << "-" << std::setfill('0') << std::setw(2) << month
       << ": (收入: CNY" << summary.income << ", 支出: CNY" << summary.expense << ")\n";
    return ss.str();
}
// --- 修改结束 ---

// ... generate_footer 和插件导出部分保持不变 ...
std::string YearRstFormat::generate_footer(const YearlyReportData& data) const {
    return "";
}

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