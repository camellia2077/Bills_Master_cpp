// windows/infrastructure/adapters/db/sqlite_report_data_gateway.hpp
#ifndef WINDOWS_INFRASTRUCTURE_ADAPTERS_DB_SQLITE_REPORT_DATA_GATEWAY_H_
#define WINDOWS_INFRASTRUCTURE_ADAPTERS_DB_SQLITE_REPORT_DATA_GATEWAY_H_

#include <sqlite3.h>

#include <string>
#include <vector>

#include "ports/report_data_gateway.hpp"

class SqliteReportDataGateway final : public ReportDataGateway {
 public:
  explicit SqliteReportDataGateway(sqlite3* db_connection);

  [[nodiscard]] auto ReadMonthlyData(int year, int month)
      -> MonthlyReportData override;
  [[nodiscard]] auto ReadYearlyData(int year) -> YearlyReportData override;
  [[nodiscard]] auto ListAvailableMonths() -> std::vector<std::string> override;

 private:
  sqlite3* db_connection_ = nullptr;
};

#endif  // WINDOWS_INFRASTRUCTURE_ADAPTERS_DB_SQLITE_REPORT_DATA_GATEWAY_H_
