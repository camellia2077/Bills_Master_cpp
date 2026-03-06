// adapters/config/TomlBillConfigLoader.cpp

#include "bills_io/adapters/config/toml_bill_config_loader.hpp"

auto TomlBillConfigLoader::Load(const toml::table& config_toml)
    -> BillConfig {
  BillValidationRules rules;
  const toml::array* categories = config_toml["categories"].as_array();
  if (categories == nullptr) {
    return BillConfig(std::move(rules));
  }

  for (const auto& category_node : *categories) {
    const toml::table* category = category_node.as_table();
    if (category == nullptr) {
      continue;
    }

    const auto* parent_title = category->get_as<std::string>("parent_item");
    if (parent_title == nullptr) {
      continue;
    }

    const std::string parent_title_value = parent_title->get();
    rules.parent_titles.insert(parent_title_value);

    std::set<std::string> sub_titles;
    if (const toml::array* sub_items = category->get_as<toml::array>("sub_items");
        sub_items != nullptr) {
      for (const auto& sub_item : *sub_items) {
        if (const auto* sub_title = sub_item.as_string()) {
          sub_titles.insert(sub_title->get());
        }
      }
    }

    rules.validation_map[parent_title_value] = std::move(sub_titles);
  }

  return BillConfig(std::move(rules));
}
