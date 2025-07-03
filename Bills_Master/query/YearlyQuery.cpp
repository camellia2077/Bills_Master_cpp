#include "YearlyQuery.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream> // **MODIFIED**: Added to build the report string

YearlyQuery::YearlyQuery(sqlite3* db_connection) : m_db(db_connection) {}

// **MODIFIED**: This method now builds and returns a string instead of printing.
std::string YearlyQuery::generate_report(const std::string& year) {
    std::stringstream ss; // Use a stringstream to build the report

    const char* sql = "SELECT bill_date, SUM(amount) FROM transactions WHERE bill_date LIKE ? GROUP BY bill_date ORDER BY bill_date;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备年度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
    }

    std::string year_pattern = year + "%";
    sqlite3_bind_text(stmt, 1, year_pattern.c_str(), -1, SQLITE_STATIC);

    std::map<std::string, double> monthly_totals;
    double grand_total = 0.0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* month_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double total_amount = sqlite3_column_double(stmt, 1);
        
        monthly_totals[month_raw] = total_amount;
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
        std::string month_str = pair.first;
        double month_total = pair.second;
        std::string formatted_month = month_str.substr(0, 4) + "-" + month_str.substr(4, 2);
        ss << " - " << formatted_month << "：" << month_total << " 元\n";
    }

    return ss.str();
}