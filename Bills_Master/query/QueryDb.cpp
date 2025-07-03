#include "QueryDb.h"
#include "YearlyQuery.h"
#include "MonthlyQuery.h"
#include <stdexcept>

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

void QueryFacade::show_yearly_summary(const std::string& year) {
    YearlyQuery query(m_db);
    query.display(year);
}

// **MODIFIED**: This method now gets the report from MonthlyQuery and returns it.
std::string QueryFacade::get_monthly_details_report(const std::string& month) {
    MonthlyQuery query(m_db);
    return query.generate_report(month);
}