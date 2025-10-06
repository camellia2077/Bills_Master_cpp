// reports/components/yearly_report/YearQuery.cpp

#include "YearQuery.hpp"
#include <stdexcept>
#include <string>

YearQuery::YearQuery(sqlite3* db_connection) : m_db(db_connection) {}

YearlyReportData YearQuery::read_yearly_data(int year) {
    YearlyReportData data;
    data.year = year;

    // --- 【核心修改】 ---
    // 更新SQL查询，以直接从 bills 表中按月分组并累加收入和支出
    const char* sql = "SELECT "
                      "  month, "
                      "  SUM(total_income), "
                      "  SUM(total_expense) "
                      "FROM bills "
                      "WHERE year = ? "
                      "GROUP BY month "
                      "ORDER BY month;";
    // --- 修改结束 ---
                      
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备年度查询的 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        data.data_found = true;
        int month = sqlite3_column_int(stmt, 0);
        double month_income = sqlite3_column_double(stmt, 1);
        double month_expense = sqlite3_column_double(stmt, 2);

        // 填充月度明细
        data.monthly_summary[month] = {month_income, month_expense};

        // 累加年度总计
        data.total_income += month_income;
        data.total_expense += month_expense;
    }
    sqlite3_finalize(stmt);

    // 计算年度总结余
    if (data.data_found) {
        data.balance = data.total_income + data.total_expense;
    }

    return data;
}