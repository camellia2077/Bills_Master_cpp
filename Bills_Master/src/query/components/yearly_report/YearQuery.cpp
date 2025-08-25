
// YearQuery.cpp
#include "YearQuery.h"
#include <stdexcept>
#include <string>

YearQuery::YearQuery(sqlite3* db_connection) : m_db(db_connection) {}

YearlyReportData YearQuery::read_yearly_data(int year) {
    YearlyReportData data;
    data.year = year;

    // --- MODIFICATION START ---
    // The SQL query now joins the transactions and bills tables.
    // We select b.month (from bills) and filter by b.year (from bills).
    const char* sql = "SELECT b.month, SUM(t.amount) "
                      "FROM transactions AS t "
                      "JOIN bills AS b ON t.bill_id = b.id "
                      "WHERE b.year = ? "
                      "GROUP BY b.month "
                      "ORDER BY b.month;";
    // --- MODIFICATION END ---
                      
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        // This error message now correctly reflects the potential failure point.
        throw std::runtime_error("Failed to prepare yearly query SQL statement: " + std::string(sqlite3_errmsg(m_db)));
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