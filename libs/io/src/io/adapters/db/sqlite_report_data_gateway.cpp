// io/adapters/db/sqlite_report_data_gateway.cpp
#include "io/adapters/db/sqlite_report_data_gateway.hpp"

#include <stdexcept>
#include <string>

#include "io/adapters/db/range_query.hpp"

namespace {
constexpr int kBillDateColumn = 0;
}  // namespace

SqliteReportDataGateway::SqliteReportDataGateway(sqlite3* db_connection)
    : db_connection_(db_connection) {
  if (db_connection_ == nullptr) {
    throw std::invalid_argument("Database connection must not be null.");
  }
}

auto SqliteReportDataGateway::ReadRangeData(std::string_view start_iso_month,
                                            std::string_view end_iso_month)
    -> RangeReportData {
  RangeQuery range_query(db_connection_);
  return range_query.read_range_data(start_iso_month, end_iso_month);
}

auto SqliteReportDataGateway::ListAvailableMonths() -> std::vector<std::string> {
  std::vector<std::string> months;
  const char* sql = "SELECT DISTINCT bill_date FROM bills ORDER BY bill_date;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, sql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare month list query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const auto* month_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, kBillDateColumn));
    if (month_text != nullptr) {
      months.emplace_back(month_text);
    }
  }

  sqlite3_finalize(stmt);
  return months;
}
