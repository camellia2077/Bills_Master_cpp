// bills_io/adapters/db/month_query.hpp
#ifndef BILLS_IO_ADAPTERS_DB_MONTH_QUERY_H_
#define BILLS_IO_ADAPTERS_DB_MONTH_QUERY_H_

#include <sqlite3.h>

#include "ports/contracts/reports/monthly/monthly_report_data.hpp"

class MonthQuery {
 public:
  explicit MonthQuery(sqlite3* db_connection);

  // 从数据库读取数据并返回一个填充好的数据结构
  MonthlyReportData read_monthly_data(int year, int month);

 private:
  sqlite3* m_db;
};

#endif

