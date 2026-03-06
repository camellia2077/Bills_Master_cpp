// config_validator/pipeline/modifier_config_validator.cpp
#include "modifier_config_validator.hpp"

namespace {
auto IsNumericNode(const toml::node& node) -> bool {
  return node.is_integer() || node.is_floating_point();
}
}  // namespace

auto ModifierConfigValidator::validate(const toml::table& config_toml,
                                       std::string& error_message) -> bool {
  if (const toml::node_view<const toml::node> auto_renewal_view =
          config_toml["auto_renewal_rules"]) {
    const toml::table* renewal_config = auto_renewal_view.as_table();
    if (renewal_config == nullptr) {
      error_message =
          "配置错误: 'auto_renewal_rules' 的值必须是表。";
      return false;
    }

    if (const toml::node_view<const toml::node> enabled_view =
            (*renewal_config)["enabled"];
        enabled_view && !enabled_view.is_boolean()) {
      error_message =
          "配置错误: 'auto_renewal_rules.enabled' 的值必须是布尔类型。";
      return false;
    }

    if (const toml::node_view<const toml::node> rules_view =
            (*renewal_config)["rules"]) {
      const toml::array* rules = rules_view.as_array();
      if (rules == nullptr) {
        error_message =
            "配置错误: 'auto_renewal_rules.rules' 的值必须是数组。";
        return false;
      }

      for (const auto& rule_node : *rules) {
        const toml::table* rule = rule_node.as_table();
        if (rule == nullptr) {
          error_message = "配置错误: 'rules' 数组中的元素必须是表。";
          return false;
        }

        if (rule->get_as<std::string>("header_location") == nullptr) {
          error_message =
              "配置错误: 'rules' 中的某个表缺少 'header_location' 字符串键。";
          return false;
        }

        const toml::node* amount = rule->get("amount");
        if (amount == nullptr || !IsNumericNode(*amount)) {
          error_message =
              "配置错误: 'rules' 中的某个表缺少 'amount' 数值键。";
          return false;
        }

        if (rule->get_as<std::string>("description") == nullptr) {
          error_message =
              "配置错误: 'rules' 中的某个表缺少 'description' 字符串键。";
          return false;
        }
      }
    }
  }

  if (const toml::node_view<const toml::node> metadata_view =
          config_toml["metadata_prefixes"]) {
    const toml::array* metadata_prefixes = metadata_view.as_array();
    if (metadata_prefixes == nullptr) {
      error_message =
          "配置错误: 'metadata_prefixes' 的值必须是字符串数组。";
      return false;
    }

    for (const auto& prefix : *metadata_prefixes) {
      if (!prefix.is_string()) {
        error_message =
            "配置错误: 'metadata_prefixes' 数组中的值必须是字符串。";
        return false;
      }
    }
  }

  if (const toml::node_view<const toml::node> display_view =
          config_toml["display_name_maps"]) {
    const toml::table* display_name_maps = display_view.as_table();
    if (display_name_maps == nullptr) {
      error_message =
          "配置错误: 'display_name_maps' 的值必须是表。";
      return false;
    }

    for (const auto& [_, map_node] : *display_name_maps) {
      const toml::table* lang_map = map_node.as_table();
      if (lang_map == nullptr) {
        error_message =
            "配置错误: 'display_name_maps' 下的每个值都必须是表。";
        return false;
      }

      for (const auto& [__, lang_value] : *lang_map) {
        if (!lang_value.is_string()) {
          error_message =
              "配置错误: 'display_name_maps' 下的语言映射值必须是字符串。";
          return false;
        }
      }
    }
  }

  return true;
}
