#include "config/config_bundle_service.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "reporting/renderers/standard_report_renderer_registry.hpp"

namespace {

constexpr const char* kValidatorSourceKind = "validator_config";
constexpr const char* kModifierSourceKind = "modifier_config";
constexpr const char* kExportSourceKind = "export_formats";

auto make_file_report(std::string source_kind, std::string path)
    -> ConfigFileValidationReport {
  ConfigFileValidationReport report;
  report.source_kind = std::move(source_kind);
  report.path = std::move(path);
  const auto slash_pos = report.path.find_last_of("/\\");
  report.file_name = slash_pos == std::string::npos
                         ? report.path
                         : report.path.substr(slash_pos + 1);
  return report;
}

auto append_issue(ConfigFileValidationReport& report, std::string stage,
                  std::string code, std::string message,
                  std::string field_path = {}, int line = 0, int column = 0) -> void {
  report.issues.push_back(
      MakeValidationIssue(report.source_kind, std::move(stage), std::move(code),
                          std::move(message), report.path, line, column,
                          std::move(field_path)));
}

auto append_parse_issue(ConfigFileValidationReport& report, std::string code,
                        const std::string& parse_error, int line,
                        int column) -> void {
  append_issue(report, "parse", std::move(code), parse_error, {}, line, column);
}

auto finalize_report(ConfigBundleValidationReport& report) -> void {
  report.processed = report.files.size();
  report.success = 0;
  report.failure = 0;
  for (auto& file : report.files) {
    file.ok = file.issues.empty();
    if (file.ok) {
      ++report.success;
    } else {
      ++report.failure;
    }
  }
  report.ok = report.failure == 0U;
}

auto validate_validator_document(const ValidatorConfigDocument& document,
                                 ConfigFileValidationReport& report,
                                 BillConfig& validator_config) -> void {
  if (!document.parsed) {
    append_parse_issue(report, "config.validator_config.parse_error",
                       document.parse_error, document.parse_error_line,
                       document.parse_error_column);
    return;
  }
  if (!document.has_categories_array || document.categories.empty()) {
    append_issue(report, "schema",
                 "config.validator_config.missing_categories",
                 "'categories' must be a non-empty array.", "categories");
    return;
  }

  BillValidationRules rules;
  for (std::size_t category_index = 0; category_index < document.categories.size();
       ++category_index) {
    const auto& category = document.categories[category_index];
    const std::string category_path =
        "categories[" + std::to_string(category_index) + "]";
    if (!category.is_table) {
      append_issue(report, "schema",
                   "config.validator_config.category_not_table",
                   "'categories' items must be tables.", category_path);
      continue;
    }
    if (!category.parent_item.has_value() || category.parent_item->empty()) {
      append_issue(report, "schema",
                   "config.validator_config.missing_parent_item",
                   "'parent_item' must be a string.",
                   category_path + ".parent_item");
      continue;
    }
    if (!category.has_sub_items_array) {
      append_issue(report, "schema",
                   "config.validator_config.missing_sub_items",
                   "'sub_items' must be an array of strings.",
                   category_path + ".sub_items");
      continue;
    }

    std::set<std::string> sub_titles;
    for (std::size_t sub_item_index = 0; sub_item_index < category.sub_items.size();
         ++sub_item_index) {
      const auto& sub_item = category.sub_items[sub_item_index];
      if (!sub_item.has_value() || sub_item->empty()) {
        append_issue(report, "schema",
                     "config.validator_config.sub_item_not_string",
                     "'sub_items' items must be strings.",
                     category_path + ".sub_items[" +
                         std::to_string(sub_item_index) + "]");
        continue;
      }
      sub_titles.insert(*sub_item);
    }

    rules.parent_titles.insert(*category.parent_item);
    rules.validation_map[*category.parent_item] = std::move(sub_titles);
  }

  if (report.issues.empty()) {
    validator_config = BillConfig(std::move(rules));
  }
}

auto validate_modifier_document(const ModifierConfigDocument& document,
                                ConfigFileValidationReport& report,
                                Config& modifier_config) -> void {
  if (!document.parsed) {
    append_parse_issue(report, "config.modifier_config.parse_error",
                       document.parse_error, document.parse_error_line,
                       document.parse_error_column);
    return;
  }

  if (!document.auto_renewal_is_table) {
    append_issue(report, "schema",
                 "config.modifier_config.auto_renewal_not_table",
                 "'auto_renewal_rules' must be a table.", "auto_renewal_rules");
  }
  if (!document.auto_renewal_rules_is_array) {
    append_issue(report, "schema",
                 "config.modifier_config.rules_not_array",
                 "'auto_renewal_rules.rules' must be an array.",
                 "auto_renewal_rules.rules");
  }
  if (!document.metadata_prefixes_is_array) {
    append_issue(report, "schema",
                 "config.modifier_config.metadata_prefixes_not_array",
                 "'metadata_prefixes' must be an array of strings.",
                 "metadata_prefixes");
  }
  if (!document.display_name_maps_is_table) {
    append_issue(report, "schema",
                 "config.modifier_config.display_name_maps_not_table",
                 "'display_name_maps' must be a table.", "display_name_maps");
  }

  for (std::size_t index = 0; index < document.auto_renewal_rules.size(); ++index) {
    const auto& rule = document.auto_renewal_rules[index];
    const std::string rule_path =
        "auto_renewal_rules.rules[" + std::to_string(index) + "]";
    if (!rule.is_table) {
      append_issue(report, "schema", "config.modifier_config.rule_not_table",
                   "'rules' items must be tables.", rule_path);
      continue;
    }
    if (!rule.header_location.has_value()) {
      append_issue(report, "schema",
                   "config.modifier_config.missing_header_location",
                   "'header_location' must be a string.",
                   rule_path + ".header_location");
    }
    if (!rule.amount.has_value()) {
      append_issue(report, "schema",
                   "config.modifier_config.missing_amount",
                   "'amount' must be numeric.", rule_path + ".amount");
    }
    if (!rule.description.has_value()) {
      append_issue(report, "schema",
                   "config.modifier_config.missing_description",
                   "'description' must be a string.",
                   rule_path + ".description");
    }
  }

  for (std::size_t index = 0; index < document.metadata_prefixes.size(); ++index) {
    const auto& prefix = document.metadata_prefixes[index];
    if (!prefix.has_value()) {
      append_issue(report, "schema",
                   "config.modifier_config.metadata_prefix_not_string",
                   "'metadata_prefixes' items must be strings.",
                   "metadata_prefixes[" + std::to_string(index) + "]");
    }
  }

  for (const auto& [map_key, lang_map] : document.display_name_maps) {
    const std::string map_path = "display_name_maps." + map_key;
    if (!lang_map.has_value()) {
      append_issue(report, "schema",
                   "config.modifier_config.display_name_entry_not_table",
                   "Each display_name_maps entry must be a table.", map_path);
      continue;
    }
    for (const auto& [lang_key, lang_value] : *lang_map) {
      if (!lang_value.has_value()) {
        append_issue(report, "schema",
                     "config.modifier_config.display_name_value_not_string",
                     "display_name_maps values must be strings.",
                     map_path + "." + lang_key);
      }
    }
  }

  if (!report.issues.empty()) {
    return;
  }

  modifier_config.auto_renewal.enabled =
      document.auto_renewal_enabled.value_or(false);
  if (modifier_config.auto_renewal.enabled) {
    for (const auto& rule : document.auto_renewal_rules) {
      modifier_config.auto_renewal.rules.push_back(
          Config::AutoRenewalRule{
              .header_location = *rule.header_location,
              .amount = *rule.amount,
              .description = *rule.description,
          });
    }
  }
  for (const auto& prefix : document.metadata_prefixes) {
    modifier_config.metadata_prefixes.push_back(*prefix);
  }
  for (const auto& [map_key, lang_map] : document.display_name_maps) {
    std::map<std::string, std::string> normalized_lang_map;
    for (const auto& [lang_key, lang_value] : *lang_map) {
      normalized_lang_map[lang_key] = *lang_value;
    }
    modifier_config.display_name_maps[map_key] = std::move(normalized_lang_map);
  }
}

auto validate_export_document(const ExportFormatsDocument& document,
                              ConfigFileValidationReport& report,
                              std::vector<std::string>& enabled_formats,
                              std::vector<std::string>& available_formats) -> void {
  available_formats = StandardReportRendererRegistry::ListAvailableFormats();
  if (!document.parsed) {
    append_parse_issue(report, "config.export_formats.parse_error",
                       document.parse_error, document.parse_error_line,
                       document.parse_error_column);
    return;
  }
  if (!document.has_enabled_formats_array) {
    append_issue(report, "schema",
                 "config.export_formats.enabled_formats_not_array",
                 "'enabled_formats' must be a non-empty array.",
                 "enabled_formats");
    return;
  }

  std::set<std::string> deduped_formats;
  for (std::size_t index = 0; index < document.enabled_formats.size(); ++index) {
    const auto& format = document.enabled_formats[index];
    const std::string field_path =
        "enabled_formats[" + std::to_string(index) + "]";
    if (!format.has_value() || format->empty()) {
      append_issue(report, "schema", "config.export_formats.item_not_string",
                   "'enabled_formats' items must be non-empty strings.",
                   field_path);
      continue;
    }

    const std::string normalized =
        StandardReportRendererRegistry::NormalizeFormat(*format);
    if (normalized.empty()) {
      append_issue(report, "business",
                   "config.export_formats.unknown_format",
                   "Format '" + *format + "' is not recognized.", field_path);
      continue;
    }
    if (!StandardReportRendererRegistry::IsFormatAvailable(normalized)) {
      append_issue(report, "business",
                   "config.export_formats.unavailable_in_build",
                   "Format '" + normalized +
                       "' is enabled in config but not available in this build.",
                   field_path);
      continue;
    }

    if (deduped_formats.insert(normalized).second) {
      enabled_formats.push_back(normalized);
    }
  }

  if (report.issues.empty() && enabled_formats.empty()) {
    append_issue(report, "schema",
                 "config.export_formats.no_enabled_formats",
                 "'enabled_formats' must contain at least one usable format.",
                 "enabled_formats");
  }
}

}  // namespace

auto ConfigBundleService::Validate(const ConfigDocumentBundle& documents)
    -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport> {
  ConfigBundleValidationReport report;
  report.files.push_back(
      make_file_report(kValidatorSourceKind, documents.validator.display_path));
  report.files.push_back(
      make_file_report(kModifierSourceKind, documents.modifier.display_path));
  report.files.push_back(
      make_file_report(kExportSourceKind, documents.export_formats.display_path));

  RuntimeConfigBundle runtime_config;
  validate_validator_document(documents.validator, report.files[0],
                              runtime_config.validator_config);
  validate_modifier_document(documents.modifier, report.files[1],
                             runtime_config.modifier_config);
  validate_export_document(documents.export_formats, report.files[2],
                           report.enabled_export_formats,
                           report.available_export_formats);

  finalize_report(report);
  if (!report.ok) {
    return std::unexpected(report);
  }

  return ValidatedConfigBundle{
      .runtime_config = std::move(runtime_config),
      .enabled_export_formats = report.enabled_export_formats,
      .available_export_formats = report.available_export_formats,
      .report = report,
  };
}
