#ifndef SQLITE_REPORT_DB_SESSION_HPP
#define SQLITE_REPORT_DB_SESSION_HPP

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

#endif  // SQLITE_REPORT_DB_SESSION_HPP
