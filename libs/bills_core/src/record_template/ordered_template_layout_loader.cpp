#include "record_template/ordered_template_layout_loader.hpp"

#include <string>
#include <utility>

#include "config_loading/runtime_config_loader.hpp"
#include "config_validator/pipeline/validator_config_validator.hpp"

auto OrderedTemplateLayoutLoader::LoadFromValidatorFile(
    const std::filesystem::path& validator_config_path)
    -> RecordTemplateResult<OrderedTemplateLayout> {
  const auto validator_toml =
      RuntimeConfigLoader::ReadTomlFile(validator_config_path);
  if (!validator_toml) {
    return std::unexpected(MakeRecordTemplateErrorFromCommon(
        validator_toml.error(), RecordTemplateErrorCategory::kConfig));
  }

  std::string error_message;
  if (!ValidatorConfigValidator::validate(*validator_toml, error_message)) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kConfig,
        "validator_config.toml invalid: " + error_message));
  }

  OrderedTemplateLayout layout;
  const toml::array* categories = (*validator_toml)["categories"].as_array();
  if (categories == nullptr) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kConfig,
        "validator_config.toml does not contain categories."));
  }

  for (const auto& category_node : *categories) {
    const toml::table* category = category_node.as_table();
    if (category == nullptr) {
      continue;
    }

    const auto* parent_item = category->get_as<std::string>("parent_item");
    if (parent_item == nullptr) {
      continue;
    }

    OrderedTemplateCategory ordered_category;
    ordered_category.parent_item = parent_item->get();
    if (const auto* description = category->get_as<std::string>("description");
        description != nullptr) {
      ordered_category.description = description->get();
    }

    if (const toml::array* sub_items =
            category->get_as<toml::array>("sub_items");
        sub_items != nullptr) {
      ordered_category.sub_items.reserve(sub_items->size());
      for (const auto& sub_item : *sub_items) {
        if (const auto* sub_item_value = sub_item.as_string()) {
          ordered_category.sub_items.push_back(sub_item_value->get());
        }
      }
    }

    layout.categories.push_back(std::move(ordered_category));
  }

  return layout;
}
