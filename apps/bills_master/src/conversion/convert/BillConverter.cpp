// conversion/convert/BillConverter.cpp

#include "BillConverter.hpp"
#include "conversion/modifier/config_loader/ConfigLoader.hpp"
#include "conversion/modifier/processor/BillContentTransformer.hpp"

BillConverter::BillConverter(const nlohmann::json& config_json) {
    m_config = ConfigLoader::load(config_json);
}

auto BillConverter::convert(const std::string& bill_content) -> ParsedBill {
    BillContentTransformer processor(m_config);
    return processor.process(bill_content);
}
