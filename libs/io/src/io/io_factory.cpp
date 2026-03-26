// io/io_factory.cpp
#include "io/io_factory.hpp"

#include <memory>
#include <utility>

#include "io/adapters/db/sqlite_bill_repository.hpp"
#include "io/adapters/db/sqlite_report_data_gateway.hpp"

namespace bills::io {

auto CreateBillRepository(std::string db_path)
    -> std::unique_ptr<BillRepository> {
  return std::make_unique<SqliteBillRepository>(std::move(db_path));
}

auto CreateReportDbSession(std::string db_path)
    -> std::unique_ptr<SqliteReportDbSession> {
  return std::make_unique<SqliteReportDbSession>(std::move(db_path));
}

auto CreateReportDataGateway(sqlite3* db_connection)
    -> std::unique_ptr<ReportDataGateway> {
  return std::make_unique<SqliteReportDataGateway>(db_connection);
}

}  // namespace bills::io
