// bills_io/adapters/db/sqlite_report_db_session.hpp
#ifndef BILLS_IO_ADAPTERS_DB_SQLITE_REPORT_DB_SESSION_H_
#define BILLS_IO_ADAPTERS_DB_SQLITE_REPORT_DB_SESSION_H_

#include <sqlite3.h>

#include <string>

class SqliteReportDbSession final {
 public:
  explicit SqliteReportDbSession(std::string db_path);
  ~SqliteReportDbSession();

  SqliteReportDbSession(const SqliteReportDbSession&) = delete;
  auto operator=(const SqliteReportDbSession&)
      -> SqliteReportDbSession& = delete;

  [[nodiscard]] auto GetConnectionHandle() const -> sqlite3*;

 private:
  sqlite3* db_connection_ = nullptr;
};

#endif  // BILLS_IO_ADAPTERS_DB_SQLITE_REPORT_DB_SESSION_H_
