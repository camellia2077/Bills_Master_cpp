// adapters/serialization/JsonBillSerializer.hpp
#ifndef JSON_BILL_SERIALIZER_HPP
#define JSON_BILL_SERIALIZER_HPP

#include "ports/bills_serializer.hpp"

class JsonBillSerializer : public BillSerializer {
 public:
  auto ReadJson(const std::string& file_path) -> ParsedBill override;
  void WriteJson(const ParsedBill& bill_data,
                 const std::string& file_path) override;
};

#endif  // JSON_BILL_SERIALIZER_HPP
