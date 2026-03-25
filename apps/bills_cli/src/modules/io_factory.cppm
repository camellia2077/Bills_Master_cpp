module;
#include "bills_io/io_factory.hpp"

export module bill.cli.deps.io_factory;

export namespace bills::io {
using ::bills::io::CreateBillRepository;
using ::bills::io::CreateReportDataGateway;
using ::bills::io::CreateReportDbSession;
}

export {
using ::BillRepository;
using ::ReportDataGateway;
using ::SqliteReportDbSession;
}
