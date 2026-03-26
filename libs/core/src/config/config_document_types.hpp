#ifndef CONFIG_CONFIG_DOCUMENT_TYPES_HPP_
#define CONFIG_CONFIG_DOCUMENT_TYPES_HPP_

#include <map>
#include <optional>
#include <string>
#include <vector>

struct ValidatorCategoryDocument {
  bool is_table = true;
  std::optional<std::string> parent_item;
  std::optional<std::string> description;
  bool has_sub_items_array = true;
  std::vector<std::optional<std::string>> sub_items;
};

struct ValidatorConfigDocument {
  std::string display_path = "validator_config.toml";
  bool parsed = false;
  bool has_categories_array = true;
  std::vector<ValidatorCategoryDocument> categories;
  std::string parse_error;
  int parse_error_line = 0;
  int parse_error_column = 0;
};

struct ModifierRuleDocument {
  bool is_table = true;
  std::optional<std::string> header_location;
  std::optional<double> amount;
  std::optional<std::string> description;
};

struct ModifierConfigDocument {
  std::string display_path = "modifier_config.toml";
  bool parsed = false;
  bool auto_renewal_is_table = true;
  std::optional<bool> auto_renewal_enabled;
  bool auto_renewal_rules_is_array = true;
  std::vector<ModifierRuleDocument> auto_renewal_rules;
  bool metadata_prefixes_is_array = true;
  std::vector<std::optional<std::string>> metadata_prefixes;
  bool display_name_maps_is_table = true;
  std::map<std::string, std::optional<std::map<std::string, std::optional<std::string>>>>
      display_name_maps;
  std::string parse_error;
  int parse_error_line = 0;
  int parse_error_column = 0;
};

struct ExportFormatsDocument {
  std::string display_path = "export_formats.toml";
  bool parsed = false;
  bool has_enabled_formats_array = true;
  std::vector<std::optional<std::string>> enabled_formats;
  std::string parse_error;
  int parse_error_line = 0;
  int parse_error_column = 0;
};

struct ConfigDocumentBundle {
  ValidatorConfigDocument validator;
  ModifierConfigDocument modifier;
  ExportFormatsDocument export_formats;
};

#endif  // CONFIG_CONFIG_DOCUMENT_TYPES_HPP_
