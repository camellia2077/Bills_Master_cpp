
#include "ConfigLoader.h"

Config ConfigLoader::load(const nlohmann::json& config_json) {
    Config config_to_populate;

    // 解析 modification_flags
    if (config_json.contains("modification_flags")) {
        const auto& flags = config_json["modification_flags"];
        config_to_populate.flags.enable_summing = flags.value("enable_summing", false);
        config_to_populate.flags.enable_cleanup = flags.value("enable_cleanup", false);
        config_to_populate.flags.enable_sorting = flags.value("enable_sorting", false);
        config_to_populate.flags.preserve_metadata_lines = flags.value("preserve_metadata_lines", false);
    }

    // 解析 formatting_rules
    if (config_json.contains("formatting_rules")) {
        const auto& formatting = config_json["formatting_rules"];
        config_to_populate.formatting.lines_after_parent_section = formatting.value("lines_after_parent_section", 1);
        config_to_populate.formatting.lines_after_parent_title = formatting.value("lines_after_parent_title", 1);
        config_to_populate.formatting.lines_between_sub_items = formatting.value("lines_between_sub_items", 1);
    }

    // 解析 auto_renewal_rules
    if (config_json.contains("auto_renewal_rules")) {
        const auto& renewal_config = config_json["auto_renewal_rules"];
        config_to_populate.auto_renewal.enabled = renewal_config.value("enabled", false);

        if (config_to_populate.auto_renewal.enabled && renewal_config.contains("rules")) {
            const auto& renewal_rules = renewal_config["rules"];
            for (auto it = renewal_rules.begin(); it != renewal_rules.end(); ++it) {
                const std::string& category = it.key();
                const nlohmann::json& items = it.value();
                for (const auto& item_json : items) {
                    config_to_populate.auto_renewal.rules[category].push_back({
                        item_json.value("amount", 0.0),
                        item_json.value("description", "")
                    });
                }
            }
        }
    }

    // 解析 metadata_prefixes
    if (config_json.contains("metadata_prefixes") && config_json["metadata_prefixes"].is_array()) {
        for (const auto& prefix_json : config_json["metadata_prefixes"]) {
            if (prefix_json.is_string()) {
                config_to_populate.metadata_prefixes.push_back(prefix_json.get<std::string>());
            }
        }
    }
    
    return config_to_populate;
}