// bills_io/adapters/config/toml_modifier_config_loader.cpp

#include "bills_io/adapters/config/toml_modifier_config_loader.hpp"

namespace {
auto ReadDouble(const toml::node* node, double fallback) -> double {
  if (node == nullptr) {
    return fallback;
  }
  if (const auto* value = node->as_floating_point()) {
    return value->get();
  }
  if (const auto* value = node->as_integer()) {
    return static_cast<double>(value->get());
  }
  return fallback;
}
}  // namespace

auto TomlModifierConfigLoader::Load(const toml::table& config_toml)
    -> Config {
  Config config_to_populate;

  if (const toml::table* renewal_config =
          config_toml["auto_renewal_rules"].as_table();
      renewal_config != nullptr) {
    if (const auto* enabled = renewal_config->get_as<bool>("enabled")) {
      config_to_populate.auto_renewal.enabled = enabled->get();
    }

    if (config_to_populate.auto_renewal.enabled) {
      if (const toml::array* rules = renewal_config->get_as<toml::array>("rules");
          rules != nullptr) {
        for (const auto& rule_node : *rules) {
          const toml::table* rule = rule_node.as_table();
          if (rule == nullptr) {
            continue;
          }

          const auto* header_location =
              rule->get_as<std::string>("header_location");
          const auto* description = rule->get_as<std::string>("description");

          config_to_populate.auto_renewal.rules.push_back(
              {header_location != nullptr ? header_location->get() : "",
               ReadDouble(rule->get("amount"), 0.0),
               description != nullptr ? description->get() : ""});
        }
      }
    }
  }

  if (const toml::array* metadata_prefixes =
          config_toml["metadata_prefixes"].as_array();
      metadata_prefixes != nullptr) {
    for (const auto& prefix_node : *metadata_prefixes) {
      if (const auto* prefix = prefix_node.as_string()) {
        config_to_populate.metadata_prefixes.push_back(prefix->get());
      }
    }
  }

  if (const toml::table* display_name_maps =
          config_toml["display_name_maps"].as_table();
      display_name_maps != nullptr) {
    for (const auto& [map_key, map_node] : *display_name_maps) {
      const toml::table* lang_table = map_node.as_table();
      if (lang_table == nullptr) {
        continue;
      }

      std::map<std::string, std::string> lang_map;
      for (const auto& [lang_key, lang_node] : *lang_table) {
        if (const auto* lang_value = lang_node.as_string()) {
          lang_map[std::string(lang_key.str())] = lang_value->get();
        }
      }

      config_to_populate.display_name_maps[std::string(map_key.str())] =
          std::move(lang_map);
    }
  }

  return config_to_populate;
}
