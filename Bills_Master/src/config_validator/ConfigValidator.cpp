// config_validator/ConfigValidator.cpp
#include "ConfigValidator.hpp"

bool ConfigValidator::validate_validator_config(const nlohmann::json& config_json, std::string& error_message) {
    if (!config_json.contains("categories") || !config_json["categories"].is_array() || config_json["categories"].empty()) {
        error_message = "配置错误: 'Validator_Config.json' 必须包含一个名为 'categories' 的非空数组。";
        return false;
    }

    for (const auto& category : config_json["categories"]) {
        if (!category.is_object()) {
            error_message = "配置错误: 'categories' 数组中的元素必须是对象。";
            return false;
        }
        if (!category.contains("parent_item") || !category["parent_item"].is_string()) {
            error_message = "配置错误: 'categories' 中的某个对象缺少 'parent_item' 字符串键。";
            return false;
        }
        if (!category.contains("sub_items") || !category["sub_items"].is_array()) {
            error_message = "配置错误: 'categories' 中的某个对象缺少 'sub_items' 数组键。";
            return false;
        }
        for (const auto& sub_item : category["sub_items"]) {
            if (!sub_item.is_string()) {
                error_message = "配置错误: 'sub_items' 数组中的某个值不是字符串。";
                return false;
            }
        }
    }
    return true;
}

bool ConfigValidator::validate_modifier_config(const nlohmann::json& config_json, std::string& error_message) {
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