// bills_io/adapters/db/year_query.hpp
#ifndef BILLS_IO_ADAPTERS_DB_YEAR_QUERY_H_
#define BILLS_IO_ADAPTERS_DB_YEAR_QUERY_H_

#include <sqlite3.h>

#include <string_view>

#include "ports/contracts/reports/yearly/yearly_report_data.hpp"

class YearQuery {
 public:
  explicit YearQuery(sqlite3* db_connection);

  // Reads yearly data and returns it in a dedicated structure.
  YearlyReportData read_yearly_data(std::string_view iso_year);

 private:
  sqlite3* m_db;
};

#endif  // BILLS_IO_ADAPTERS_DB_YEAR_QUERY_H_
