
#include "BillMetadataReader.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>

BillMetadataReader::BillMetadataReader(sqlite3* db_connection) : m_db(db_connection) {}

// 这是从 QueryDb.cpp 中完整迁移过来的代码
std::vector<std::string> BillMetadataReader::get_all_bill_dates() {
    std::vector<std::string> dates;
    const char* sql = "SELECT DISTINCT year, month FROM bills ORDER BY year, month;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare SQL statement to get all dates: " + std::string(sqlite3_errmsg(m_db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int year = sqlite3_column_int(stmt, 0);
        int month = sqlite3_column_int(stmt, 1);
        
        std::stringstream ss;
        ss << year << std::setfill('0') << std::setw(2) << month;
        dates.push_back(ss.str());
    }

    sqlite3_finalize(stmt);
    return dates;
}