// bills_io/adapters/db/sqlite_report_data_gateway.cpp
#include "bills_io/adapters/db/sqlite_report_data_gateway.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bills_io/adapters/db/month_query.hpp"
#include "bills_io/adapters/db/year_query.hpp"

namespace {
constexpr int kYearColumn = 0;
constexpr int kMonthColumn = 1;
}  // namespace

SqliteReportDataGateway::SqliteReportDataGateway(sqlite3* db_connection)
    : db_connection_(db_connection) {
  if (db_connection_ == nullptr) {
    throw std::invalid_argument("Database connection must not be null.");
  }
}

auto SqliteReportDataGateway::ReadMonthlyData(int year, int month)
    -> MonthlyReportData {
  MonthQuery month_query(db_connection_);
  return month_query.read_monthly_data(year, month);
}

auto SqliteReportDataGateway::ReadYearlyData(int year) -> YearlyReportData {
  YearQuery year_query(db_connection_);
  return year_query.read_yearly_data(year);
}

auto SqliteReportDataGateway::ListAvailableMonths() -> std::vector<std::string> {
  std::vector<std::string> months;
  const char* sql =
      "SELECT DISTINCT year, month FROM bills ORDER BY year, month;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, sql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare month list query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int year = sqlite3_column_int(stmt, kYearColumn);
    int month = sqlite3_column_int(stmt, kMonthColumn);

    std::stringstream month_stream;
    month_stream << year << "-" << std::setfill('0') << std::setw(2) << month;
    months.push_back(month_stream.str());
  }

  sqlite3_finalize(stmt);
  return months;
}
