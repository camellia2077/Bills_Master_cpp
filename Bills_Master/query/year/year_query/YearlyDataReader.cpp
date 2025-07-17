// YearlyDataReader.cpp
#include "YearlyDataReader.h"
#include <stdexcept>
#include <string>

YearlyDataReader::YearlyDataReader(sqlite3* db_connection) : m_db(db_connection) {}

YearlyReportData YearlyDataReader::read_yearly_data(int year) {
    YearlyReportData data;
    data.year = year;

    const char* sql = "SELECT month, SUM(amount) FROM transactions WHERE year = ? GROUP BY month ORDER BY month;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备年度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        data.data_found = true;
        int month = sqlite3_column_int(stmt, 0);
        double total_amount = sqlite3_column_double(stmt, 1);

        data.monthly_totals[month] = total_amount;
        data.grand_total += total_amount;
    }
    sqlite3_finalize(stmt);

    return data;
}