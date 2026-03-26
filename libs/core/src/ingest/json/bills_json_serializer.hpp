// ingest/json/bills_json_serializer.hpp
#ifndef SERIALIZATION_BILLS_JSON_SERIALIZER_H_
#define SERIALIZATION_BILLS_JSON_SERIALIZER_H_

#include <string>

#include "domain/bill/bill_record.hpp"
#include "nlohmann/json.hpp"

class BillJsonSerializer {
 public:
  static ParsedBill deserialize(const std::string& json_text);
  static std::string serialize(const ParsedBill& bill_data);

 private:
  static ParsedBill deserialize_json(const nlohmann::json& data);
  static nlohmann::ordered_json to_json(const ParsedBill& bill_data);
};

#endif  // SERIALIZATION_BILLS_JSON_SERIALIZER_H_
