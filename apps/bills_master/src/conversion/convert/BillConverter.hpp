// conversion/convert/BillConverter.hpp
#ifndef BILL_CONVERTER_HPP
#define BILL_CONVERTER_HPP

#include "common/structures/CommonData.hpp"
#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"
#include <string>

class BillConverter {
public:
    explicit BillConverter(const nlohmann::json& config_json);

    ParsedBill convert(const std::string& bill_content);

private:
    Config m_config;
};

#endif // BILL_CONVERTER_HPP
