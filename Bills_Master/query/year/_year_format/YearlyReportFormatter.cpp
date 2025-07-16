// YearlyReportFormatter.cpp
#include "YearlyReportFormatter.h"
#include <sstream>
#include <iomanip>

std::string YearlyReportFormatter::format_report(const YearlyReportData& data) {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "\n未找到 " << data.year << " 年的任何数据。\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);
    ss << "\n## " << data.year << "年总支出：**" << data.grand_total << " 元**\n";
    ss << "\n## 每月支出\n";

    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << " - " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val << "：" << month_total << " 元\n";
    }

    return ss.str();
}