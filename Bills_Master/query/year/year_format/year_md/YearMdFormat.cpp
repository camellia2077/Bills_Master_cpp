// YearMdFormat.cpp
#include "YearMdFormat.h"
#include <sstream>
#include <iomanip>

// 构造函数实现：初始化配置成员
YearMdFormat::YearMdFormat(const YearMdConfig& config) : config(config) {}

std::string YearMdFormat::format_report(const YearlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2;
        return ss.str();
    }

    ss << std::fixed << std::setprecision(config.precision);

    // --- 年度总计标题：硬编码 '##', ':**' 和 '**'，使用配置项组合标签 ---
    ss << "\n## " << data.year << config.yearly_total_label << ":**" 
       << data.grand_total << " " << config.currency_name << "**\n";

    // --- 每月支出标题：硬编码 '## ' 前缀 ---
    ss << "\n## " << config.monthly_breakdown_title << "\n";

    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        
        // --- 每月条目：硬编码 ' - ' 前缀，并使用通用的 currency_name ---
        ss << " - " << data.year << config.monthly_item_date_separator
           << std::setfill(config.fill_char) << std::setw(config.month_width) << month_val 
           << config.monthly_item_value_separator << month_total << " " << config.currency_name << "\n";
    }

    return ss.str();
}