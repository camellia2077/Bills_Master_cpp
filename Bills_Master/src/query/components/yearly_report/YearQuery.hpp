// YearQuery.hpp
#ifndef YEARLY_DATA_READER_H
#define YEARLY_DATA_READER_H

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

#endif // YEARLY_DATA_READER_H