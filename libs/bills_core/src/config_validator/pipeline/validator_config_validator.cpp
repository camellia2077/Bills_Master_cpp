// config_validator/pipeline/validator_config_validator.cpp
#include "validator_config_validator.hpp"

auto ValidatorConfigValidator::validate(const toml::table& config_toml,
                                        std::string& error_message) -> bool {
  const toml::array* categories = config_toml["categories"].as_array();
  if (categories == nullptr || categories->empty()) {
    error_message =
        "配置错误: 'validator_config.toml' 必须包含一个名为 'categories' 的非空数组。";
    return false;
  }

  for (const auto& category_node : *categories) {
    const toml::table* category = category_node.as_table();
    if (category == nullptr) {
      error_message = "配置错误: 'categories' 数组中的元素必须是表。";
      return false;
    }

    if (category->get_as<std::string>("parent_item") == nullptr) {
      error_message =
          "配置错误: 'categories' 中的某个表缺少 'parent_item' 字符串键。";
      return false;
    }

    const toml::array* sub_items = category->get_as<toml::array>("sub_items");
    if (sub_items == nullptr) {
      error_message =
          "配置错误: 'categories' 中的某个表缺少 'sub_items' 字符串数组键。";
      return false;
    }

    for (const auto& sub_item : *sub_items) {
      if (!sub_item.is_string()) {
        error_message =
            "配置错误: 'sub_items' 数组中的某个值不是字符串。";
        return false;
      }
    }
  }

  return true;
}
