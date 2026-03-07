// bills_io/adapters/db/sqlite_bill_repository.hpp
#ifndef BILLS_IO_ADAPTERS_DB_SQLITE_BILL_REPOSITORY_H_
#define BILLS_IO_ADAPTERS_DB_SQLITE_BILL_REPOSITORY_H_

#include <string>

#include "ports/bills_repository.hpp"

class SqliteBillRepository : public BillRepository {
 public:
  explicit SqliteBillRepository(std::string db_path);
  void InsertBill(const ParsedBill& bill_data) override;

 private:
  std::string db_path_;
};

#endif  // BILLS_IO_ADAPTERS_DB_SQLITE_BILL_REPOSITORY_H_
