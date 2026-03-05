// adapters/config/JsonModifierConfigLoader.cpp

#include "bills_io/adapters/config/json_modifier_config_loader.hpp"

auto JsonModifierConfigLoader::Load(const nlohmann::json& config_json)
    -> Config {
  Config config_to_populate;

  if (config_json.contains("auto_renewal_rules")) {
    const auto& renewal_config = config_json["auto_renewal_rules"];
    config_to_populate.auto_renewal.enabled =
        renewal_config.value("enabled", false);

    if (config_to_populate.auto_renewal.enabled &&
        renewal_config.contains("rules") &&
        renewal_config["rules"].is_array()) {
      for (const auto& rule_json : renewal_config["rules"]) {
        config_to_populate.auto_renewal.rules.push_back(
            {rule_json.value("header_location", ""),
             rule_json.value("amount", 0.0),
             rule_json.value("description", "")});
      }
    }
  }

  if (config_json.contains("metadata_prefixes") &&
      config_json["metadata_prefixes"].is_array()) {
    for (const auto& prefix_json : config_json["metadata_prefixes"]) {
      if (prefix_json.is_string()) {
        config_to_populate.metadata_prefixes.push_back(
            prefix_json.get<std::string>());
      }
    }
  }

  if (config_json.contains("display_name_maps") &&
      config_json["display_name_maps"].is_object()) {
    for (auto map_it = config_json["display_name_maps"].begin();
         map_it != config_json["display_name_maps"].end(); ++map_it) {
      if (map_it.value().is_object()) {
        std::map<std::string, std::string> lang_map;
        for (auto lang_it = map_it.value().begin();
             lang_it != map_it.value().end(); ++lang_it) {
          if (lang_it.value().is_string()) {
            lang_map[lang_it.key()] = lang_it.value();
          }
        }
        config_to_populate.display_name_maps[map_it.key()] = lang_map;
      }
    }
  }

  return config_to_populate;
}
