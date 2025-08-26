// MonthQuery.hpp
#ifndef MONTH_QUERY_H
#define MONTH_QUERY_H


#include "ReportData.hpp"

#include <sqlite3.h>

class MonthQuery {
public:
    explicit MonthQuery(sqlite3* db_connection);

    // 从数据库读取数据并返回一个填充好的数据结构
    MonthlyReportData read_monthly_data(int year, int month);

private:
    sqlite3* m_db;
};

#endif 