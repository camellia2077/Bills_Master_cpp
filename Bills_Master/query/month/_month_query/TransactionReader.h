// TransactionReader.h
#ifndef TRANSACTION_READER_H
#define TRANSACTION_READER_H

#include <sqlite3.h>
#include "month/_month_data/ReportData.h"

class TransactionReader {
public:
    explicit TransactionReader(sqlite3* db_connection);

    // 从数据库读取数据并返回一个填充好的数据结构
    MonthlyReportData read_monthly_data(int year, int month);

private:
    sqlite3* m_db;
};

#endif // TRANSACTION_READER_H