#ifndef MONTHLY_QUERY_H
#define MONTHLY_QUERY_H

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include "parser.h" // 仍然使用 Transaction 结构体

// -- 新增的数据结构 --

// 用于存储子类别的数据，包括小计总额和其下的所有交易
struct SubCategoryData {
    double sub_total = 0.0;
    std::vector<Transaction> transactions;
};

// 用于存储父类别的数据，包括总计金额和其下的所有子类别
struct ParentCategoryData {
    double parent_total = 0.0;
    std::map<std::string, SubCategoryData> sub_categories;
};


class MonthlyQuery {
public:
    explicit MonthlyQuery(sqlite3* db_connection);

    // **MODIFIED**: This method now returns the report as a string instead of printing it.
    // **MODIFIED**: The method signature is changed to accept year and month as integers.
    std::string generate_report(int year, int month);

private:
    sqlite3* m_db;
};

#endif // MONTHLY_QUERY_H