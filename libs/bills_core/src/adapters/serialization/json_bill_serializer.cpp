// adapters/serialization/JsonBillSerializer.cpp

#include "json_bill_serializer.hpp"

#include "serialization/bills_json_serializer.hpp"

auto JsonBillSerializer::ReadJson(const std::string& file_path) -> ParsedBill {
  return BillJsonSerializer::read_from_file(file_path);
}

void JsonBillSerializer::WriteJson(const ParsedBill& bill_data,
                                   const std::string& file_path) {
  BillJsonSerializer::write_to_file(bill_data, file_path);
}
