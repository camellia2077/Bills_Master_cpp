#include "QueryDb.h"
#include "YearlyQuery.h"
#include "MonthlyQuery.h"
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>

// --- Constructor / Destructor (Assumed to be the same) ---
QueryFacade::QueryFacade(const std::string& db_path) : m_db(nullptr) {
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("无法以只读模式打开数据库: " + errmsg);
    }
}

QueryFacade::~QueryFacade() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

// --- MODIFIED Report Generation Methods ---
std::string QueryFacade::get_yearly_summary_report(int year) {
    YearlyQuery query(m_db);
    return query.generate_report(year);
}

std::string QueryFacade::get_monthly_details_report(int year, int month) {
    MonthlyQuery query(m_db);
    return query.generate_report(year, month);
}


// --- MODIFIED Method Implementation ---
std::vector<std::string> QueryFacade::get_all_bill_dates() {
    std::vector<std::string> dates;
    const char* sql = "SELECT DISTINCT year, month FROM transactions ORDER BY year, month;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("准备查询所有日期 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
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