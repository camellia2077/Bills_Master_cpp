// ports/bills_serializer.hpp
#ifndef PORTS_BILLS_SERIALIZER_H_
#define PORTS_BILLS_SERIALIZER_H_

#include <string>

#include "domain/bill/bill_record.hpp"

class BillSerializer {
 public:
  virtual ~BillSerializer() = default;
  virtual auto ReadJson(const std::string& file_path) -> ParsedBill = 0;
  virtual void WriteJson(const ParsedBill& bill_data,
                         const std::string& file_path) = 0;
};

#endif  // PORTS_BILLS_SERIALIZER_H_


