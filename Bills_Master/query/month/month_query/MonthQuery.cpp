// MonthQuery.cpp
#include "MonthQuery.h"
#include <stdexcept>
#include <string>

MonthQuery::MonthQuery(sqlite3* db_connection) : m_db(db_connection) {}

MonthlyReportData MonthQuery::read_monthly_data(int year, int month) {
    MonthlyReportData data;
    data.year = year;
    data.month = month;

    const char* sql = "SELECT t.parent_category, t.sub_category, t.amount, t.description, b.remark "
                  "FROM transactions AS t "
                  "JOIN bills AS b ON t.bill_id = b.id "
                  "WHERE b.year = ? AND b.month = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);
    sqlite3_bind_int(stmt, 2, month);

    bool first_row = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        data.data_found = true; // 标记找到了数据

        std::string parent_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string sub_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        double amount = sqlite3_column_double(stmt, 2);
        const unsigned char* desc_raw = sqlite3_column_text(stmt, 3);

        Transaction t;
        t.parent_category = parent_cat;
        t.sub_category = sub_cat;
        t.amount = amount;
        t.description = desc_raw ? reinterpret_cast<const char*>(desc_raw) : "";

        // 聚合数据
        data.aggregated_data[parent_cat].parent_total += amount;
        data.aggregated_data[parent_cat].sub_categories[sub_cat].sub_total += amount;
        data.aggregated_data[parent_cat].sub_categories[sub_cat].transactions.push_back(t);

        data.grand_total += amount;

        // remark 对于一个月的所有交易都是相同的，所以只取一次
        if (first_row) {
            const unsigned char* remark_raw = sqlite3_column_text(stmt, 4);
            data.remark = remark_raw ? reinterpret_cast<const char*>(remark_raw) : "";
            first_row = false;
        }
    }
    sqlite3_finalize(stmt);

    return data;
}