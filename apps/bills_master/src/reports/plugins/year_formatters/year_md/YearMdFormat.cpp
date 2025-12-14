// reports/plugins/year_formatters/year_md/YearMdFormat.cpp

#include "YearMdFormat.hpp"
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

// --- 【核心修改】: 更新摘要部分 ---
std::string YearMdFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config.precision);
    ss << "\n## " << data.year << "年 总览\n";
    ss << "- **" << config.yearly_income_label << ":** " << data.total_income << " " << config.currency_name << "\n";
    ss << "- **" << config.yearly_expense_label << ":** " << data.total_expense << " " << config.currency_name << "\n";
    ss << "- **" << config.yearly_balance_label << ":** " << data.balance << " " << config.currency_name << "\n";
    return ss.str();
}

std::string YearMdFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    ss << "\n## " << config.monthly_breakdown_title << "\n\n"
       << "| " << config.monthly_table_header_month 
       << " | " << config.monthly_table_header_income << " (" << config.currency_name << ")"// 收入
       << " | " << config.monthly_table_header_expense << " (" << config.currency_name << ")"// 支出
       << " | " << config.monthly_table_header_balance << " (" << config.currency_name << ") |\n" // 新增结余列名
       << "| :--- | :--- | :--- | :--- |\n";
    return ss.str();
}

std::string YearMdFormat::generate_monthly_item(int year, int month, const MonthlySummary& summary) const {
    std::stringstream ss;
    // 计算当月结余 (收入 + 支出，因为支出是负数)
    double monthly_balance = summary.income + summary.expense;

    ss << std::fixed << std::setprecision(config.precision);
    ss << "| " << year << config.monthly_item_date_separator
       << std::setfill(config.fill_char) << std::setw(config.month_width) << month
       << " | " << summary.income 
       << " | " << summary.expense 
       << " | " << monthly_balance << " |\n"; // 输出计算结果
    return ss.str();
}

std::string YearMdFormat::generate_footer(const YearlyReportData& data) const {
    return ""; // Markdown 没有正式的尾部
}

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