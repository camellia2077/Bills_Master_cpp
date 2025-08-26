// ReportData.hpp
#ifndef REPORT_DATA_H
#define REPORT_DATA_H

#include <string>
#include <vector>
#include <map>
#include "db_insert/parser/BillParser.hpp" //Transaction 结构体在此定义

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

// 核心数据结构，用于在读取器和格式化器之间传递数据
struct MonthlyReportData {
    int year;
    int month;
    double grand_total = 0.0;
    std::string remark;
    // 使用 map 来存储聚合后的数据
    std::map<std::string, ParentCategoryData> aggregated_data;
    bool data_found = false;
};

#endif // REPORT_DATA_H