// bills_io/io_factory.cpp
#include "bills_io/io_factory.hpp"

#include <memory>
#include <utility>

#include "bills_io/adapters/config/toml_config_provider.hpp"
#include "bills_io/adapters/db/sqlite_bill_repository.hpp"
#include "bills_io/adapters/db/sqlite_report_data_gateway.hpp"
#include "bills_io/adapters/io/file_bill_content_reader.hpp"
#include "bills_io/adapters/io/file_bill_file_enumerator.hpp"
#include "bills_io/adapters/io/year_partition_output_path_builder.hpp"
#include "adapters/serialization/json_bill_serializer.hpp"

namespace bills::io {

auto CreateBillContentReader() -> std::unique_ptr<BillContentReader> {
  return std::make_unique<FileBillContentReader>();
}

auto CreateBillFileEnumerator() -> std::unique_ptr<BillFileEnumerator> {
  return std::make_unique<FileBillFileEnumerator>();
}

auto CreateBillSerializer() -> std::unique_ptr<BillSerializer> {
  return std::make_unique<JsonBillSerializer>();
}

auto CreateBillRepository(std::string db_path)
    -> std::unique_ptr<BillRepository> {
  return std::make_unique<SqliteBillRepository>(std::move(db_path));
}

auto CreateYearPartitionOutputPathBuilder(std::string base_output_dir)
    -> std::unique_ptr<OutputPathBuilder> {
  return std::make_unique<YearPartitionOutputPathBuilder>(
      std::move(base_output_dir));
}

auto CreateConfigProvider() -> std::unique_ptr<ConfigProvider> {
  return std::make_unique<TomlConfigProvider>();
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
