// adapters/config/JsonBillConfigLoader.hpp
#ifndef JSON_BILL_CONFIG_LOADER_HPP
#define JSON_BILL_CONFIG_LOADER_HPP

#include "billing/conversion/validator/config/bills_config.hpp"
#include "nlohmann/json.hpp"

class JsonBillConfigLoader {
 public:
  static auto Load(const nlohmann::json& config_data) -> BillConfig;
};

#endif  // JSON_BILL_CONFIG_LOADER_HPP


