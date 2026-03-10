#include "config_loading/runtime_config_loader.hpp"

#include <map>
#include <set>
#include <string>
#include <utility>

#include "config_validator/pipeline/modifier_config_validator.hpp"
#include "config_validator/pipeline/validator_config_validator.hpp"

namespace {

constexpr const char* kConfigContext = "RuntimeConfigLoader";
constexpr const char* kValidatorConfigName = "validator_config.toml";
constexpr const char* kModifierConfigName = "modifier_config.toml";

auto ReadDouble(const toml::node* node, double fallback) -> double {
  if (node == nullptr) {
    return fallback;
  }
  if (const auto* floating_value = node->as_floating_point()) {
    return floating_value->get();
  }
  if (const auto* integer_value = node->as_integer()) {
    return static_cast<double>(integer_value->get());
  }
  return fallback;
}

}  // namespace

auto RuntimeConfigLoader::ReadTomlFile(const std::filesystem::path& file_path)
    -> Result<toml::table> {
  try {
    if (!std::filesystem::is_regular_file(file_path)) {
      return std::unexpected(
          MakeError("Failed to open config file: " + file_path.string(),
                    kConfigContext));
    }
    return toml::parse_file(file_path.string());
  } catch (const std::exception& error) {
    return std::unexpected(MakeError(error.what(), kConfigContext));
  }
}

auto RuntimeConfigLoader::LoadFromConfigDir(
    const std::filesystem::path& config_dir) -> Result<RuntimeConfigBundle> {
  return LoadFromFiles(config_dir / kValidatorConfigName,
                       config_dir / kModifierConfigName);
}

auto RuntimeConfigLoader::LoadFromFiles(
    const std::filesystem::path& validator_config_path,
    const std::filesystem::path& modifier_config_path)
    -> Result<RuntimeConfigBundle> {
  const auto validator_toml = ReadTomlFile(validator_config_path);
  if (!validator_toml) {
    return std::unexpected(validator_toml.error());
  }

  const auto modifier_toml = ReadTomlFile(modifier_config_path);
  if (!modifier_toml) {
    return std::unexpected(modifier_toml.error());
  }

  const auto validator_config = LoadValidatorConfig(*validator_toml);
  if (!validator_config) {
    return std::unexpected(validator_config.error());
  }

  const auto modifier_config = LoadModifierConfig(*modifier_toml);
  if (!modifier_config) {
    return std::unexpected(modifier_config.error());
  }

  return RuntimeConfigBundle{
      .validator_config = std::move(*validator_config),
      .modifier_config = std::move(*modifier_config),
  };
}

auto RuntimeConfigLoader::LoadValidatorConfig(const toml::table& validator_toml)
    -> Result<BillConfig> {
  std::string error_message;
  if (!ValidatorConfigValidator::validate(validator_toml, error_message)) {
    return std::unexpected(MakeError(
        "validator_config.toml invalid: " + error_message, kConfigContext));
  }

  BillValidationRules rules;
  if (const toml::array* categories = validator_toml["categories"].as_array();
      categories != nullptr) {
    for (const auto& category_node : *categories) {
      const toml::table* category = category_node.as_table();
      if (category == nullptr) {
        continue;
      }

      const auto* parent_item = category->get_as<std::string>("parent_item");
      if (parent_item == nullptr) {
        continue;
      }

      const std::string parent_item_value = parent_item->get();
      rules.parent_titles.insert(parent_item_value);

      std::set<std::string> sub_items_to_validate;
      if (const toml::array* sub_items = category->get_as<toml::array>("sub_items");
          sub_items != nullptr) {
        for (const auto& sub_item : *sub_items) {
          if (const auto* sub_item_value = sub_item.as_string()) {
            sub_items_to_validate.insert(sub_item_value->get());
          }
        }
      }

      rules.validation_map[parent_item_value] = std::move(sub_items_to_validate);
    }
  }

  return BillConfig(std::move(rules));
}

auto RuntimeConfigLoader::LoadModifierConfig(const toml::table& modifier_toml)
    -> Result<Config> {
  std::string error_message;
  if (!ModifierConfigValidator::validate(modifier_toml, error_message)) {
    return std::unexpected(MakeError(
        "modifier_config.toml invalid: " + error_message, kConfigContext));
  }

  Config config;
  if (const toml::table* renewal_config =
          modifier_toml["auto_renewal_rules"].as_table();
      renewal_config != nullptr) {
    if (const auto* enabled = renewal_config->get_as<bool>("enabled")) {
      config.auto_renewal.enabled = enabled->get();
    }

    if (config.auto_renewal.enabled) {
      if (const toml::array* rules = renewal_config->get_as<toml::array>("rules");
          rules != nullptr) {
        for (const auto& rule_node : *rules) {
          const toml::table* rule = rule_node.as_table();
          if (rule == nullptr) {
            continue;
          }

          const auto* header_location =
              rule->get_as<std::string>("header_location");
          const auto* description = rule->get_as<std::string>("description");
          config.auto_renewal.rules.push_back(
              {header_location != nullptr ? header_location->get() : "",
               ReadDouble(rule->get("amount"), 0.0),
               description != nullptr ? description->get() : ""});
        }
      }
    }
  }

  if (const toml::array* metadata_prefixes =
          modifier_toml["metadata_prefixes"].as_array();
      metadata_prefixes != nullptr) {
    for (const auto& prefix_node : *metadata_prefixes) {
      if (const auto* prefix = prefix_node.as_string()) {
        config.metadata_prefixes.push_back(prefix->get());
      }
    }
  }

  if (const toml::table* display_name_maps =
          modifier_toml["display_name_maps"].as_table();
      display_name_maps != nullptr) {
    for (const auto& [map_key, map_node] : *display_name_maps) {
      const toml::table* lang_table = map_node.as_table();
      if (lang_table == nullptr) {
        continue;
      }

      std::map<std::string, std::string> lang_map;
      for (const auto& [lang_key, lang_node] : *lang_table) {
        if (const auto* lang_value = lang_node.as_string()) {
          lang_map[std::string(lang_key.str())] = lang_value->get();
        }
      }
      config.display_name_maps[std::string(map_key.str())] = std::move(lang_map);
    }
  }

  return config;
}
