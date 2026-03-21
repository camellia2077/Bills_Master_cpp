// bills_io/io_factory.hpp
#ifndef BILLS_IO_IO_FACTORY_H_
#define BILLS_IO_IO_FACTORY_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bills_io/adapters/db/sqlite_report_db_session.hpp"
#include "ports/bills_repository.hpp"
#include "ports/report_data_gateway.hpp"

namespace bills::io {

[[nodiscard]] auto CreateBillRepository(std::string db_path)
    -> std::unique_ptr<BillRepository>;
[[nodiscard]] auto CreateReportDbSession(std::string db_path)
    -> std::unique_ptr<SqliteReportDbSession>;
[[nodiscard]] auto CreateReportDataGateway(sqlite3* db_connection)
    -> std::unique_ptr<ReportDataGateway>;

}  // namespace bills::io

#endif  // BILLS_IO_IO_FACTORY_H_
