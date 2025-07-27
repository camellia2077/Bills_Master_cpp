// YearRstFormat.cpp
#include "YearRstFormat.h"
#include <sstream>
#include <iomanip>

std::string YearRstFormat::format_report(const YearlyReportData& data) const{
    std::stringstream ss;

    // --- 标题 ---
    std::string title = std::to_string(data.year) + "年 消费总览";
    ss << title << "\n";
    // RST 标题的下划线，长度需要和标题一致
    ss << std::string(title.length() * 2, '=') << "\n\n"; 

    if (!data.data_found) {
        ss << "未找到 " << data.year << " 年的任何数据。\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);

    // --- 摘要部分 ---
    ss << "**年度总支出:** CNY" << data.grand_total << "\n\n";
    
    // --- 每月支出详情 (二级标题) ---
    std::string subtitle = "每月支出";
    ss << subtitle << "\n";
    ss << std::string(subtitle.length() * 2, '-') << "\n\n";

    // 使用无序列表
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "* " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << ": CNY" << month_total << "\n";
    }

    return ss.str();
}