// adapters/db/SqliteBillRepository.cpp

#include "sqlite_bill_repository.hpp"

#include "bills_io/adapters/db/bill_inserter.hpp"

SqliteBillRepository::SqliteBillRepository(std::string db_path)
    : db_path_(std::move(db_path)) {}

void SqliteBillRepository::InsertBill(const ParsedBill& bill_data) {
  BillInserter inserter(db_path_);
  inserter.insert_bill(bill_data);
}
