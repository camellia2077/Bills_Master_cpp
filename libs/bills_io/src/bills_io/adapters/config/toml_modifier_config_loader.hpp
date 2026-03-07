// bills_io/adapters/config/toml_modifier_config_loader.hpp
#ifndef BILLS_IO_ADAPTERS_CONFIG_TOML_MODIFIER_CONFIG_LOADER_H_
#define BILLS_IO_ADAPTERS_CONFIG_TOML_MODIFIER_CONFIG_LOADER_H_

#include <toml++/toml.hpp>

#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"

class TomlModifierConfigLoader {
 public:
  static auto Load(const toml::table& config_toml) -> Config;
};

#endif  // BILLS_IO_ADAPTERS_CONFIG_TOML_MODIFIER_CONFIG_LOADER_H_
