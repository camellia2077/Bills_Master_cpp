// windows/infrastructure/adapters/db/sqlite_report_db_session.cpp
#include "windows/infrastructure/adapters/db/sqlite_report_db_session.hpp"

#include <stdexcept>

SqliteReportDbSession::SqliteReportDbSession(std::string db_path) {
  if (sqlite3_open_v2(db_path.c_str(), &db_connection_, SQLITE_OPEN_READONLY,
                      nullptr) == SQLITE_OK) {
    return;
  }

  std::string error_message = "Cannot open database.";
  if (db_connection_ != nullptr) {
    error_message = "Cannot open database: " +
                    std::string(sqlite3_errmsg(db_connection_));
    sqlite3_close(db_connection_);
    db_connection_ = nullptr;
  }

  throw std::runtime_error(error_message);
}

SqliteReportDbSession::~SqliteReportDbSession() {
  if (db_connection_ != nullptr) {
    sqlite3_close(db_connection_);
  }
}

auto SqliteReportDbSession::GetConnectionHandle() const -> sqlite3* {
  return db_connection_;
}

