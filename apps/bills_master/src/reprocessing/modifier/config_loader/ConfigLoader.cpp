// reprocessing/modifier/config_loader/ConfigLoader.cpp

#include "ConfigLoader.hpp"

Config ConfigLoader::load(const nlohmann::json& config_json) {
    Config config_to_populate;

    // --- 解析自动续费规则 ---
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

    // --- 解析元数据前缀 ---
    if (config_json.contains("metadata_prefixes") && config_json["metadata_prefixes"].is_array()) {
        for (const auto& prefix_json : config_json["metadata_prefixes"]) {
            if (prefix_json.is_string()) {
                config_to_populate.metadata_prefixes.push_back(prefix_json.get<std::string>());
            }
        }
    }

    // --- 解析多语言显示名称映射 ---
    if (config_json.contains("display_name_maps") && config_json["display_name_maps"].is_object()) {
        for (auto it = config_json["display_name_maps"].begin(); it != config_json["display_name_maps"].end(); ++it) {
            if (it.value().is_object()) {
                std::map<std::string, std::string> lang_map;
                for (auto lang_it = it.value().begin(); lang_it != it.value().end(); ++lang_it) {
                    if (lang_it.value().is_string()) {
                        lang_map[lang_it.key()] = lang_it.value();
                    }
                }
                config_to_populate.display_name_maps[it.key()] = lang_map;
            }
        }
    }
    
    return config_to_populate;
}