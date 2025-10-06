// reports/components/yearly_report/YearlyReportData.hpp
#ifndef YEARLY_REPORT_DATA_HPP
#define YEARLY_REPORT_DATA_HPP

#include <map>

// --- 【核心修改 1】 ---
// 新增一个结构体，用于存储每个月的收入和支出
struct MonthlySummary {
    double income = 0.0;
    double expense = 0.0;
};
// --- 修改结束 ---

// 用于存储年度报告所需的所有数据
struct YearlyReportData {
    int year;
    bool data_found = false;

    // --- 【核心修改 2】 ---
    // 使用新的总计字段替换 grand_total
    double total_income = 0.0;
    double total_expense = 0.0;
    double balance = 0.0;

    // 使用新的 MonthlySummary 结构体来存储月度明细
    std::map<int, MonthlySummary> monthly_summary;
    // --- 修改结束 ---
};

#endif // YEARLY_REPORT_DATA_HPP