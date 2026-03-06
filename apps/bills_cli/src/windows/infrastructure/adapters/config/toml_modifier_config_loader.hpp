// adapters/config/TomlModifierConfigLoader.hpp
#ifndef TOML_MODIFIER_CONFIG_LOADER_HPP
#define TOML_MODIFIER_CONFIG_LOADER_HPP

#include <toml++/toml.hpp>

#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"

class TomlModifierConfigLoader {
 public:
  static auto Load(const toml::table& config_toml) -> Config;
};

#endif  // TOML_MODIFIER_CONFIG_LOADER_HPP
