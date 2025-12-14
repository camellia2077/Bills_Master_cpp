// reports/plugins/year_formatters/year_rst/YearRstFormat.cpp

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
    ss << "**年总收入:** CNY" << data.total_income << "\n";
    ss << "**年总支出:** CNY" << data.total_expense << "\n";
    ss << "**年终结余:** CNY" << data.balance << "\n\n";
    return ss.str();
}

// --- 【核心修改 1】: 使用 list-table 指令生成表格表头 ---
std::string YearRstFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    // 使用 list-table 指令，避免了手动对齐字符宽度的困难
    ss << "\n.. list-table:: 每月明细\n";
    ss << "   :widths: 15 25 25 25\n"; // 设置列宽比例
    ss << "   :header-rows: 1\n\n";    // 标记第一行为表头，注意这里必须有一个空行
    
    // 输出表头项
    ss << "   * - 月份\n";
    ss << "     - 收入\n";
    ss << "     - 支出\n";
    ss << "     - 结余\n";
    return ss.str();
}

// --- 【核心修改 2】: 计算结余并输出为表格行 ---
std::string YearRstFormat::generate_monthly_item(int year, int month, const MonthlySummary& summary) const {
    std::stringstream ss;
    // 计算当月结余
    double balance = summary.income + summary.expense;

    ss << std::fixed << std::setprecision(2);
    
    // 按照 list-table 的列表格式输出行
    // 注意：每一行都必须以 "   * -" 开头（对应 list-table 的缩进）
    ss << "   * - " << year << "-" << std::setfill('0') << std::setw(2) << month << "\n";
    ss << "     - CNY " << summary.income << "\n";
    ss << "     - CNY " << summary.expense << "\n";
    ss << "     - CNY " << balance << "\n";
    return ss.str();
}

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