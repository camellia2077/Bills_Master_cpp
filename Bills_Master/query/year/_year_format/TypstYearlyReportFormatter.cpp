#include "TypstYearlyReportFormatter.h"
#include <sstream>
#include <iomanip>

std::string TypstYearlyReportFormatter::format_report(const YearlyReportData& data) {
    std::stringstream ss;

    // --- 设置文档参数和标题 ---
    ss << "#set document(title: \"" << data.year << "年消费总览\", author: \"BillsMaster\")\n";
    ss << "#set text(font: \"Noto Serif SC\", size: 12pt)\n\n";
    
    ss << "= " << data.year << "年 消费总览\n\n";

    if (!data.data_found) {
        ss << "未找到 " << data.year << " 年的任何数据。\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);

    // --- 摘要部分 ---
    ss << "== 年度总览\n\n";
    ss << "  - *年度总支出：* ¥" << data.grand_total << "\n\n";
    
    // --- 每月支出详情 ---
    ss << "== 每月支出\n\n";
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "  - " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << "：¥" << month_total << "\n";
    }

    return ss.str();
}