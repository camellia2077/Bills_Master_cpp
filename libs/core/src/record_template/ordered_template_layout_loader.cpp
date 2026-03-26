#include "record_template/ordered_template_layout_loader.hpp"

#include <utility>

auto OrderedTemplateLayoutLoader::BuildFromDocument(
    const ValidatorConfigDocument& validator_document)
    -> RecordTemplateResult<OrderedTemplateLayout> {
  if (!validator_document.parsed) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kConfig, validator_document.parse_error));
  }
  if (!validator_document.has_categories_array ||
      validator_document.categories.empty()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kConfig,
        "validator_config.toml does not contain categories."));
  }

  OrderedTemplateLayout layout;
  for (const auto& category : validator_document.categories) {
    if (!category.is_table || !category.parent_item.has_value()) {
      continue;
    }

    OrderedTemplateCategory ordered_category;
    ordered_category.parent_item = *category.parent_item;
    if (category.description.has_value()) {
      ordered_category.description = *category.description;
    }
    for (const auto& sub_item : category.sub_items) {
      if (sub_item.has_value()) {
        ordered_category.sub_items.push_back(*sub_item);
      }
    }

    layout.categories.push_back(std::move(ordered_category));
  }

  return layout;
}
