// config_validator/pipeline/ModifierConfigValidator.cpp
#include "ModifierConfigValidator.hpp"

bool ModifierConfigValidator::validate(const nlohmann::json& config_json, std::string& error_message) {
    if (config_json.contains("auto_renewal_rules")) {
        const auto& renewal_config = config_json["auto_renewal_rules"];
        if (!renewal_config.is_object()) {
            error_message = "配置错误: 'auto_renewal_rules' 的值必须是一个对象。";
            return false;
        }
        if (renewal_config.contains("enabled") && !renewal_config["enabled"].is_boolean()) {
             error_message = "配置错误: 'auto_renewal_rules' 对象中的 'enabled' 键值必须是布尔类型。";
            return false;
        }
        if (renewal_config.contains("rules")) {
            if(!renewal_config["rules"].is_array()){
                error_message = "配置错误: 'auto_renewal_rules' 对象中的 'rules' 键值必须是数组。";
                return false;
            }
            for (const auto& rule : renewal_config["rules"]) {
                if (!rule.is_object()) {
                    error_message = "配置错误: 'rules' 数组的元素必须是对象。";
                    return false;
                }
                if (!rule.contains("header_location") || !rule["header_location"].is_string()) {
                    error_message = "配置错误: 'rules' 对象缺少 'header_location' 字符串键。";
                    return false;
                }
                if (!rule.contains("amount") || !rule["amount"].is_number()) {
                    error_message = "配置错误: 'rules' 对象缺少 'amount' 数字键。";
                    return false;
                }
                if (!rule.contains("description") || !rule["description"].is_string()) {
                    error_message = "配置错误: 'rules' 对象缺少 'description' 字符串键。";
                    return false;
                }
            }
        }
    }

    if (config_json.contains("metadata_prefixes") && !config_json["metadata_prefixes"].is_array()) {
        error_message = "配置错误: 'metadata_prefixes' 的值必须是字符串数组。";
        return false;
    }

    if (config_json.contains("display_name_maps") && !config_json["display_name_maps"].is_object()) {
        error_message = "配置错误: 'display_name_maps' 的值必须是对象。";
        return false;
    }
    return true;
}