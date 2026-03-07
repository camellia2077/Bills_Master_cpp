// adapters/serialization/json_bill_serializer.hpp
#ifndef ADAPTERS_SERIALIZATION_JSON_BILL_SERIALIZER_H_
#define ADAPTERS_SERIALIZATION_JSON_BILL_SERIALIZER_H_

#include "ports/bills_serializer.hpp"

class JsonBillSerializer : public BillSerializer {
 public:
  auto ReadJson(const std::string& file_path) -> ParsedBill override;
  void WriteJson(const ParsedBill& bill_data,
                 const std::string& file_path) override;
};

#endif  // ADAPTERS_SERIALIZATION_JSON_BILL_SERIALIZER_H_
