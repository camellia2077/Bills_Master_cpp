// serialization/BillJsonSerializer.hpp
#ifndef BILL_JSON_SERIALIZER_HPP
#define BILL_JSON_SERIALIZER_HPP

#include "common/structures/CommonData.hpp"
#include "nlohmann/json.hpp"
#include <string>

class BillJsonSerializer {
public:
    static ParsedBill read_from_file(const std::string& file_path);
    static std::string serialize(const ParsedBill& bill_data);
    static void write_to_file(const ParsedBill& bill_data, const std::string& file_path);

private:
    static ParsedBill deserialize(const nlohmann::json& data);
    static nlohmann::ordered_json to_json(const ParsedBill& bill_data);
};

#endif // BILL_JSON_SERIALIZER_HPP
