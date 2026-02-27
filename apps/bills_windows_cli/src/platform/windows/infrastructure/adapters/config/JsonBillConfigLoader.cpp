// adapters/config/JsonBillConfigLoader.cpp

#include "JsonBillConfigLoader.hpp"

auto JsonBillConfigLoader::Load(const nlohmann::json& config_data)
    -> BillConfig {
  BillValidationRules rules;
  for (const auto& category : config_data.at("categories")) {
    const std::string kParentTitle =
        category.at("parent_item").get<std::string>();
    rules.parent_titles.insert(kParentTitle);

    std::set<std::string> sub_titles;
    const auto& sub_items = category.at("sub_items");
    if (sub_items.is_array()) {
      for (const auto& sub : sub_items) {
        sub_titles.insert(sub.get<std::string>());
      }
    }
    rules.validation_map[kParentTitle] = std::move(sub_titles);
  }
  return BillConfig(std::move(rules));
}
