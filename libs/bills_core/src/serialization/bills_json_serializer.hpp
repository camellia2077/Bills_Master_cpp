// serialization/bills_json_serializer.hpp
#ifndef SERIALIZATION_BILLS_JSON_SERIALIZER_H_
#define SERIALIZATION_BILLS_JSON_SERIALIZER_H_

#include <string>

#include "domain/bill/bill_record.hpp"
#include "nlohmann/json.hpp"

class BillJsonSerializer {
 public:
  static ParsedBill read_from_file(const std::string& file_path);
  static std::string serialize(const ParsedBill& bill_data);
  static void write_to_file(const ParsedBill& bill_data,
                            const std::string& file_path);

 private:
  static ParsedBill deserialize(const nlohmann::json& data);
  static nlohmann::ordered_json to_json(const ParsedBill& bill_data);
};

#endif  // SERIALIZATION_BILLS_JSON_SERIALIZER_H_


