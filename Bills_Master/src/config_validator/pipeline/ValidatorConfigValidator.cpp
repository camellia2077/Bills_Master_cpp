// config_validator/pipeline/ValidatorConfigValidator.cpp
#include "ValidatorConfigValidator.hpp"

bool ValidatorConfigValidator::validate(const nlohmann::json& config_json, std::string& error_message) {
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