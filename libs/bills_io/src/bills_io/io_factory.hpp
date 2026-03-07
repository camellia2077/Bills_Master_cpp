// bills_io/io_factory.hpp
#ifndef BILLS_IO_IO_FACTORY_H_
#define BILLS_IO_IO_FACTORY_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bills_io/adapters/db/sqlite_report_db_session.hpp"
#include "ports/bills_repository.hpp"
#include "ports/bills_content_reader.hpp"
#include "ports/bills_file_enumerator.hpp"
#include "ports/bills_serializer.hpp"
#include "ports/config_provider.hpp"
#include "ports/month_report_formatter_provider.hpp"
#include "ports/report_data_gateway.hpp"
#include "ports/yearly_report_formatter_provider.hpp"
#include "application/use_cases/workflow_use_case.hpp"
#include "reports/core/report_export_service.hpp"

namespace bills::io {

[[nodiscard]] auto CreateBillContentReader() -> std::unique_ptr<BillContentReader>;
[[nodiscard]] auto CreateBillFileEnumerator()
    -> std::unique_ptr<BillFileEnumerator>;
[[nodiscard]] auto CreateBillSerializer() -> std::unique_ptr<BillSerializer>;
[[nodiscard]] auto CreateBillRepository(std::string db_path)
    -> std::unique_ptr<BillRepository>;
[[nodiscard]] auto CreateConfigProvider() -> std::unique_ptr<ConfigProvider>;
[[nodiscard]] auto CreateReportDbSession(std::string db_path)
    -> std::unique_ptr<SqliteReportDbSession>;
[[nodiscard]] auto CreateReportDataGateway(sqlite3* db_connection)
    -> std::unique_ptr<ReportDataGateway>;
[[nodiscard]] auto CreateMonthReportFormatterProvider()
    -> std::unique_ptr<MonthReportFormatterProvider>;
[[nodiscard]] auto CreateYearlyReportFormatterProvider()
    -> std::unique_ptr<YearlyReportFormatterProvider>;

}  // namespace bills::io

#endif  // BILLS_IO_IO_FACTORY_H_
