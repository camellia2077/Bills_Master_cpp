// ports/BillSerializer.hpp
#ifndef BILL_SERIALIZER_HPP
#define BILL_SERIALIZER_HPP

#include <string>

#include "common/structures/CommonData.hpp"

class BillSerializer {
 public:
  virtual ~BillSerializer() = default;
  virtual auto ReadJson(const std::string& file_path) -> ParsedBill = 0;
  virtual void WriteJson(const ParsedBill& bill_data,
                         const std::string& file_path) = 0;
};

#endif  // BILL_SERIALIZER_HPP
