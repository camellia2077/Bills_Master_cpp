// reprocessing/modifier/config_loader/ConfigLoader.cpp

#include "ConfigLoader.hpp"

Config ConfigLoader::load(const nlohmann::json& config_json) {
    Config config_to_populate;

    // --- 修改：重写对 auto_renewal_rules 的解析逻辑 ---
    if (config_json.contains("auto_renewal_rules")) {
        const auto& renewal_config = config_json["auto_renewal_rules"];
        config_to_populate.auto_renewal.enabled = renewal_config.value("enabled", false);

        // 检查 "rules" 是否存在并且是一个数组
        if (config_to_populate.auto_renewal.enabled && renewal_config.contains("rules") && renewal_config["rules"].is_array()) {
            // 遍历JSON数组中的每一个对象
            for (const auto& rule_json : renewal_config["rules"]) {
                // 将解析出的数据填充到新的 AutoRenewalRule 结构体中
                config_to_populate.auto_renewal.rules.push_back({
                    rule_json.value("header_location", ""),
                    rule_json.value("amount", 0.0),
                    rule_json.value("description", "")
                });
            }
        }
    }

    // 解析 metadata_prefixes (保持不变)
    if (config_json.contains("metadata_prefixes") && config_json["metadata_prefixes"].is_array()) {
        for (const auto& prefix_json : config_json["metadata_prefixes"]) {
            if (prefix_json.is_string()) {
                config_to_populate.metadata_prefixes.push_back(prefix_json.get<std::string>());
            }
        }
    }
    
    return config_to_populate;
}