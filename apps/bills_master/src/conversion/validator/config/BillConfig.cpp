// conversion/validator/config/BillConfig.cpp

#include "BillConfig.hpp"

BillConfig::BillConfig(BillValidationRules rules)
    : validation_map_(std::move(rules.validation_map)),
      all_parent_titles_(std::move(rules.parent_titles)) {}

auto BillConfig::is_parent_title(const std::string& title) const -> bool {
  return all_parent_titles_.contains(title);
}

auto BillConfig::is_valid_sub_title(const std::string& parent_title,
                                    const std::string& sub_title) const
    -> bool {
  if (!validation_map_.contains(parent_title)) {
    return false;
  }
  const auto& sub_titles = validation_map_.at(parent_title);
  return sub_titles.contains(sub_title);
}
