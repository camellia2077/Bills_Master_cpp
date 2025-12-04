// reports/monthly_report/MonthQuery.cpp

#include "MonthQuery.hpp"
#include <stdexcept>
#include <string>

MonthQuery::MonthQuery(sqlite3* db_connection) : m_db(db_connection) {}

MonthlyReportData MonthQuery::read_monthly_data(int year, int month) {
    MonthlyReportData data;
    data.year = year;
    data.month = month;

    // --- 【核心修改 1】: 首先，查询总计数据 ---
    const char* totals_sql = "SELECT total_income, total_expense, balance, remark FROM bills WHERE year = ? AND month = ?;";
    sqlite3_stmt* totals_stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, totals_sql, -1, &totals_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备查询总计数据的 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }
    sqlite3_bind_int(totals_stmt, 1, year);
    sqlite3_bind_int(totals_stmt, 2, month);

    if (sqlite3_step(totals_stmt) == SQLITE_ROW) {
        data.data_found = true; // 只要能查到 bills 记录，就认为有数据
        data.total_income = sqlite3_column_double(totals_stmt, 0);
        data.total_expense = sqlite3_column_double(totals_stmt, 1);
        data.balance = sqlite3_column_double(totals_stmt, 2);
        const unsigned char* remark_raw = sqlite3_column_text(totals_stmt, 3);
        data.remark = remark_raw ? reinterpret_cast<const char*>(remark_raw) : "";
    }
    sqlite3_finalize(totals_stmt);
    // --- 修改结束 ---


    // 如果没有找到月份记录，直接返回
    if (!data.data_found) {
        return data;
    }

    // --- 【核心修改 2】: 其次，查询所有交易记录 ---
    const char* sql = "SELECT t.parent_category, t.sub_category, t.amount, t.description "
                      "FROM transactions AS t "
                      "JOIN bills AS b ON t.bill_id = b.id "
                      "WHERE b.year = ? AND b.month = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备查询交易的 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);
    sqlite3_bind_int(stmt, 2, month);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
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

        // grand_total 的累加不再需要，因为我们已经直接从 bills 表中获取了总数
    }
    sqlite3_finalize(stmt);
    // --- 修改结束 ---

    return data;
}