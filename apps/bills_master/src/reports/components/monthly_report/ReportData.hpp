// reports/components/monthly_report/ReportData.hpp
#ifndef REPORT_DATA_HPP
#define REPORT_DATA_HPP

#include <string>
#include <vector>
#include <map>
#include "common_structures/CommonData.hpp" 

// 用于存储子类别的数据
struct SubCategoryData {
    double sub_total = 0.0;
    std::vector<Transaction> transactions;
};

// 用于存储父类别的数据
struct ParentCategoryData {
    double parent_total = 0.0;
    std::map<std::string, SubCategoryData> sub_categories;
};

// 用于存储每月汇总报告的数据
struct MonthlyReportData {
    int year;
    int month;
    std::string remark;
    std::map<std::string, ParentCategoryData> aggregated_data;
    bool data_found = false;

    // --- 【核心修改】 ---
    // 移除了 grand_total
    // double grand_total = 0.0;
    // 添加了三个新的总计字段
    double total_income = 0.0;
    double total_expense = 0.0;
    double balance = 0.0;
    // --- 修改结束 ---
};

#endif // REPORT_DATA_HPP