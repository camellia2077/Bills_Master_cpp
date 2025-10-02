// query/components/monthly_report/ReportData.hpp
#ifndef REPORT_DATA_HPP
#define REPORT_DATA_HPP

#include <string>
#include <vector>
#include <map>
// --- 核心修改 ---
// 不再依赖 db_insert 模块，而是包含新的通用数据结构头文件
#include "common_structures/CommonData.hpp" 

// 用于存储子类别的数据
struct SubCategoryData {
    double sub_total = 0.0;
    std::vector<Transaction> transactions; // <--- 现在 Transaction 的定义来自 CommonData.hpp
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
    double grand_total = 0.0;
    std::string remark;
    std::map<std::string, ParentCategoryData> aggregated_data;
    bool data_found = false;
};

#endif // REPORT_DATA_HPP