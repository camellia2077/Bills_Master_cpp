// adapters/config/TomlBillConfigLoader.hpp
#ifndef TOML_BILL_CONFIG_LOADER_HPP
#define TOML_BILL_CONFIG_LOADER_HPP

#include <toml++/toml.hpp>

#include "billing/conversion/validator/config/bills_config.hpp"

class TomlBillConfigLoader {
 public:
  static auto Load(const toml::table& config_toml) -> BillConfig;
};

#endif  // TOML_BILL_CONFIG_LOADER_HPP
