// ports/bills_repository.hpp
#ifndef PORTS_BILLS_REPOSITORY_H_
#define PORTS_BILLS_REPOSITORY_H_

#include "domain/bill/bill_record.hpp"

class BillRepository {
 public:
  virtual ~BillRepository() = default;
  virtual void InsertBill(const ParsedBill& bill_data) = 0;
};

#endif  // PORTS_BILLS_REPOSITORY_H_


