// YearlyDataReader.h
#ifndef YEARLY_DATA_READER_H
#define YEARLY_DATA_READER_H

#include <sqlite3.h>
#include "YearlyReportData.h"

class YearlyDataReader {
public:
    explicit YearlyDataReader(sqlite3* db_connection);

    // Reads yearly data and returns it in a dedicated structure.
    YearlyReportData read_yearly_data(int year);

private:
    sqlite3* m_db;
};

#endif // YEARLY_DATA_READER_H