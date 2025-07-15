#include "YearlyQuery.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream> // **MODIFIED**: Added to build the report string

YearlyQuery::YearlyQuery(sqlite3* db_connection) : m_db(db_connection) {}

// **MODIFIED**: This method now builds and returns a string based on an integer year.
std::string YearlyQuery::generate_report(int year) {
    std::stringstream ss; // Use a stringstream to build the report

    const char* sql = "SELECT month, SUM(amount) FROM transactions WHERE year = ? GROUP BY month ORDER BY month;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备年度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    sqlite3_bind_int(stmt, 1, year);

    std::map<int, double> monthly_totals;
    double grand_total = 0.0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int month = sqlite3_column_int(stmt, 0);
        double total_amount = sqlite3_column_double(stmt, 1);
        
        monthly_totals[month] = total_amount;
        grand_total += total_amount;
    }
    sqlite3_finalize(stmt);

    if (monthly_totals.empty()) {
        ss << "\n未找到 " << year << " 年的任何数据。\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);
    ss << "\n## " << year << "年总支出：**" << grand_total << " 元**\n";
    ss << "\n## 每月支出\n";

    for (const auto& pair : monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << " - " << year << "-" << std::setfill('0') << std::setw(2) << month_val << "：" << month_total << " 元\n";
    }

    return ss.str();
}