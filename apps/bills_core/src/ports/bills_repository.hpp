// ports/bills_repository.hpp
#ifndef BILL_REPOSITORY_HPP
#define BILL_REPOSITORY_HPP

#include "domain/bill/bill_record.hpp"

class BillRepository {
 public:
  virtual ~BillRepository() = default;
  virtual void InsertBill(const ParsedBill& bill_data) = 0;
};

#endif  // BILL_REPOSITORY_HPP


