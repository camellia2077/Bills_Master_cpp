// bills_io/adapters/config/toml_bill_config_loader.hpp
#ifndef BILLS_IO_ADAPTERS_CONFIG_TOML_BILL_CONFIG_LOADER_H_
#define BILLS_IO_ADAPTERS_CONFIG_TOML_BILL_CONFIG_LOADER_H_

#include <toml++/toml.hpp>

#include "billing/conversion/validator/config/bills_config.hpp"

class TomlBillConfigLoader {
 public:
  static auto Load(const toml::table& config_toml) -> BillConfig;
};

#endif  // BILLS_IO_ADAPTERS_CONFIG_TOML_BILL_CONFIG_LOADER_H_
