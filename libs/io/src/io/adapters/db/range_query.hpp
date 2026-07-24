#ifndef BILLS_IO_ADAPTERS_DB_RANGE_QUERY_H_
#define BILLS_IO_ADAPTERS_DB_RANGE_QUERY_H_

#include <sqlite3.h>

#include <string_view>

#include "ports/contracts/reports/range/range_report_data.hpp"

class RangeQuery {
 public:
  explicit RangeQuery(sqlite3* db_connection);

  [[nodiscard]] auto read_range_data(std::string_view start_iso_month,
                                     std::string_view end_iso_month)
      -> RangeReportData;

 private:
  sqlite3* db_connection_ = nullptr;
};

#endif  // BILLS_IO_ADAPTERS_DB_RANGE_QUERY_H_
