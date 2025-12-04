// reports/yearly_report/YearQuery.hpp
#ifndef YEAR_QUERY_HPP
#define YEAR_QUERY_HPP

#include "YearlyReportData.hpp"

#include <sqlite3.h>

class YearQuery {
public:
    explicit YearQuery(sqlite3* db_connection);

    // Reads yearly data and returns it in a dedicated structure.
    YearlyReportData read_yearly_data(int year);

private:
    sqlite3* m_db;
};

#endif // YEAR_QUERY_HPP