// reprocessing/modifier/config_loader/ConfigLoader.cpp

#include "ConfigLoader.hpp"

Config ConfigLoader::load(const nlohmann::json& config_json) {
    Config config_to_populate;

    if (config_json.contains("auto_renewal_rules")) {
        const auto& renewal_config = config_json["auto_renewal_rules"];
        config_to_populate.auto_renewal.enabled = renewal_config.value("enabled", false);

        if (config_to_populate.auto_renewal.enabled && renewal_config.contains("rules") && renewal_config["rules"].is_array()) {
            for (const auto& rule_json : renewal_config["rules"]) {
                config_to_populate.auto_renewal.rules.push_back({
                    rule_json.value("header_location", ""),
                    rule_json.value("amount", 0.0),
                    rule_json.value("description", "")
                });
            }
        }
    }

    if (config_json.contains("metadata_prefixes") && config_json["metadata_prefixes"].is_array()) {
        for (const auto& prefix_json : config_json["metadata_prefixes"]) {
            if (prefix_json.is_string()) {
                config_to_populate.metadata_prefixes.push_back(prefix_json.get<std::string>());
            }
        }
    }

    // 新增: 解析 parent_item 显示名称的映射
    if (config_json.contains("parent_item_display_names") && config_json["parent_item_display_names"].is_object()) {
        for (auto it = config_json["parent_item_display_names"].begin(); it != config_json["parent_item_display_names"].end(); ++it) {
            if (it.value().is_string()) {
                config_to_populate.parent_item_display_names[it.key()] = it.value();
            }
        }
    }
    
    return config_to_populate;
}