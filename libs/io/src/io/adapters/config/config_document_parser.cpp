#include "io/adapters/config/config_document_parser.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include <toml++/toml.hpp>

namespace {
constexpr const char* kContext = "ConfigDocumentParser";

auto read_text_file(const std::filesystem::path& file_path) -> Result<std::string> {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    return std::unexpected(
        MakeError("Failed to open config file: " + file_path.string(), kContext));
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

template <typename TDocument>
auto apply_parse_error(TDocument& document, const toml::parse_error& error) -> void {
  document.parsed = false;
  document.parse_error = error.what();
  document.parse_error_line = static_cast<int>(error.source().begin.line);
  document.parse_error_column = static_cast<int>(error.source().begin.column);
}

auto parse_validator_document(std::string_view raw_text,
                              const std::string& display_path)
    -> ValidatorConfigDocument {
  ValidatorConfigDocument document;
  document.display_path = display_path;
  try {
    const toml::table table = toml::parse(raw_text, display_path);
    document.parsed = true;
    const toml::array* categories = table["categories"].as_array();
    document.has_categories_array = categories != nullptr;
    if (categories == nullptr) {
      return document;
    }

    for (const auto& category_node : *categories) {
      ValidatorCategoryDocument category;
      category.is_table = category_node.is_table();
      const toml::table* category_table = category_node.as_table();
      if (category_table != nullptr) {
        if (const auto* parent_item =
                category_table->get_as<std::string>("parent_item")) {
          category.parent_item = parent_item->get();
        }
        if (const auto* description =
                category_table->get_as<std::string>("description")) {
          category.description = description->get();
        }
        const toml::array* sub_items = category_table->get_as<toml::array>("sub_items");
        category.has_sub_items_array = sub_items != nullptr;
        if (sub_items != nullptr) {
          for (const auto& sub_item : *sub_items) {
            if (const auto* item = sub_item.as_string()) {
              category.sub_items.push_back(item->get());
            } else {
              category.sub_items.push_back(std::nullopt);
            }
          }
        }
      } else {
        category.has_sub_items_array = false;
      }
      document.categories.push_back(std::move(category));
    }
  } catch (const toml::parse_error& error) {
    apply_parse_error(document, error);
  } catch (const std::exception& error) {
    document.parsed = false;
    document.parse_error = error.what();
  }
  return document;
}

auto parse_modifier_document(std::string_view raw_text,
                             const std::string& display_path)
    -> ModifierConfigDocument {
  ModifierConfigDocument document;
  document.display_path = display_path;
  try {
    const toml::table table = toml::parse(raw_text, display_path);
    document.parsed = true;

    if (const auto auto_renewal_view = table["auto_renewal_rules"]) {
      document.auto_renewal_is_table = auto_renewal_view.is_table();
      const toml::table* renewal_table = auto_renewal_view.as_table();
      if (renewal_table != nullptr) {
        if (const auto* enabled = renewal_table->get_as<bool>("enabled")) {
          document.auto_renewal_enabled = enabled->get();
        }
        if (const auto rules_view = (*renewal_table)["rules"]) {
          document.auto_renewal_rules_is_array = rules_view.is_array();
          const toml::array* rules = rules_view.as_array();
          if (rules != nullptr) {
            for (const auto& rule_node : *rules) {
              ModifierRuleDocument rule;
              rule.is_table = rule_node.is_table();
              const toml::table* rule_table = rule_node.as_table();
              if (rule_table != nullptr) {
                if (const auto* header_location =
                        rule_table->get_as<std::string>("header_location")) {
                  rule.header_location = header_location->get();
                }
                const toml::node* amount = rule_table->get("amount");
                if (amount != nullptr) {
                  if (const auto* floating_value = amount->as_floating_point()) {
                    rule.amount = floating_value->get();
                  } else if (const auto* integer_value = amount->as_integer()) {
                    rule.amount = static_cast<double>(integer_value->get());
                  }
                }
                if (const auto* description =
                        rule_table->get_as<std::string>("description")) {
                  rule.description = description->get();
                }
              }
              document.auto_renewal_rules.push_back(std::move(rule));
            }
          }
        }
      }
    }

    if (const auto metadata_view = table["metadata_prefixes"]) {
      document.metadata_prefixes_is_array = metadata_view.is_array();
      const toml::array* metadata_prefixes = metadata_view.as_array();
      if (metadata_prefixes != nullptr) {
        for (const auto& prefix : *metadata_prefixes) {
          if (const auto* prefix_value = prefix.as_string()) {
            document.metadata_prefixes.push_back(prefix_value->get());
          } else {
            document.metadata_prefixes.push_back(std::nullopt);
          }
        }
      }
    }

    if (const auto display_name_maps_view = table["display_name_maps"]) {
      document.display_name_maps_is_table = display_name_maps_view.is_table();
      const toml::table* display_name_maps = display_name_maps_view.as_table();
      if (display_name_maps != nullptr) {
        for (const auto& [map_key, map_node] : *display_name_maps) {
          if (!map_node.is_table()) {
            document.display_name_maps[std::string(map_key.str())] = std::nullopt;
            continue;
          }
          std::map<std::string, std::optional<std::string>> lang_map;
          for (const auto& [lang_key, lang_value] : *map_node.as_table()) {
            if (const auto* string_value = lang_value.as_string()) {
              lang_map[std::string(lang_key.str())] = string_value->get();
            } else {
              lang_map[std::string(lang_key.str())] = std::nullopt;
            }
          }
          document.display_name_maps[std::string(map_key.str())] =
              std::move(lang_map);
        }
      }
    }
  } catch (const toml::parse_error& error) {
    apply_parse_error(document, error);
  } catch (const std::exception& error) {
    document.parsed = false;
    document.parse_error = error.what();
  }
  return document;
}

auto parse_export_formats_document(std::string_view raw_text,
                                   const std::string& display_path)
    -> ExportFormatsDocument {
  ExportFormatsDocument document;
  document.display_path = display_path;
  try {
    const toml::table table = toml::parse(raw_text, display_path);
    document.parsed = true;
    const toml::array* enabled_formats = table["enabled_formats"].as_array();
    document.has_enabled_formats_array = enabled_formats != nullptr;
    if (enabled_formats != nullptr) {
      for (const auto& format : *enabled_formats) {
        if (const auto* value = format.as_string()) {
          document.enabled_formats.push_back(value->get());
        } else {
          document.enabled_formats.push_back(std::nullopt);
        }
      }
    }
  } catch (const toml::parse_error& error) {
    apply_parse_error(document, error);
  } catch (const std::exception& error) {
    document.parsed = false;
    document.parse_error = error.what();
  }
  return document;
}

}  // namespace

auto ConfigDocumentParser::ParseFiles(
    const std::filesystem::path& validator_config_path,
    const std::filesystem::path& modifier_config_path,
    const std::filesystem::path& export_formats_path)
    -> Result<ConfigDocumentBundle> {
  const auto validator_text = read_text_file(validator_config_path);
  if (!validator_text) {
    return std::unexpected(validator_text.error());
  }
  const auto modifier_text = read_text_file(modifier_config_path);
  if (!modifier_text) {
    return std::unexpected(modifier_text.error());
  }
  const auto export_text = read_text_file(export_formats_path);
  if (!export_text) {
    return std::unexpected(export_text.error());
  }
  return ParseTexts(*validator_text, *modifier_text, *export_text,
                    validator_config_path.string(), modifier_config_path.string(),
                    export_formats_path.string());
}

auto ConfigDocumentParser::ParseTexts(
    std::string_view validator_config_text, std::string_view modifier_config_text,
    std::string_view export_formats_text, const std::string& validator_display_path,
    const std::string& modifier_display_path,
    const std::string& export_formats_display_path) -> ConfigDocumentBundle {
  return ConfigDocumentBundle{
      .validator =
          parse_validator_document(validator_config_text, validator_display_path),
      .modifier =
          parse_modifier_document(modifier_config_text, modifier_display_path),
      .export_formats = parse_export_formats_document(export_formats_text,
                                                      export_formats_display_path),
  };
}
