// adapters/config/JsonModifierConfigLoader.hpp
#ifndef JSON_MODIFIER_CONFIG_LOADER_HPP
#define JSON_MODIFIER_CONFIG_LOADER_HPP

#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"

class JsonModifierConfigLoader {
 public:
  static auto Load(const nlohmann::json& config_json) -> Config;
};

#endif  // JSON_MODIFIER_CONFIG_LOADER_HPP
