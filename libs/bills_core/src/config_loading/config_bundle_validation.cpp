#include "config_loading/config_bundle_validation.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include <toml++/toml.hpp>

namespace {
namespace fs = std::filesystem;

constexpr const char* kValidatorSourceKind = "validator_config";
constexpr const char* kModifierSourceKind = "modifier_config";

struct ParsedTomlDocument {
  bool ok = false;
  toml::table table;
  std::vector<ValidationIssue> issues;
};

auto ReadTextFile(const fs::path& file_path) -> std::string {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open config file: " + file_path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto MakeConfigReport(std::string source_kind, std::string file_name,
                      const fs::path& path) -> ConfigFileValidationReport {
  return ConfigFileValidationReport{
      .source_kind = std::move(source_kind),
      .file_name = std::move(file_name),
      .path = path,
  };
}

auto AppendIssue(ConfigFileValidationReport& report, std::string stage,
                 std::string code, std::string message,
                 std::string field_path = {}, int line = 0,
                 int column = 0) -> void {
  report.issues.push_back(MakeValidationIssue(
      report.source_kind, std::move(stage), std::move(code),
      std::move(message), report.path, line, column, std::move(field_path)));
}

auto ParseTomlText(std::string_view raw_text, const fs::path& display_path,
                   std::string_view source_kind) -> ParsedTomlDocument {
  ParsedTomlDocument result;
  try {
    result.table = toml::parse(raw_text, display_path.string());
    result.ok = true;
  } catch (const toml::parse_error& error) {
    result.issues.push_back(MakeValidationIssue(
        std::string(source_kind), "parse",
        "config." + std::string(source_kind) + ".parse_error", error.what(),
        display_path, static_cast<int>(error.source().begin.line),
        static_cast<int>(error.source().begin.column)));
  } catch (const std::exception& error) {
    result.issues.push_back(MakeValidationIssue(
        std::string(source_kind), "parse",
        "config." + std::string(source_kind) + ".parse_failed", error.what(),
        display_path));
  }
  return result;
}

auto ValidateValidatorToml(const toml::table& config_toml,
                           ConfigFileValidationReport& report) -> void {
  const toml::array* categories = config_toml["categories"].as_array();
  if (categories == nullptr || categories->empty()) {
    AppendIssue(report, "schema",
                "config.validator_config.missing_categories",
                "'categories' must be a non-empty array.", "categories");
    return;
  }

  for (std::size_t category_index = 0; category_index < categories->size();
       ++category_index) {
    const toml::node& category_node = (*categories)[category_index];
    const toml::table* category = category_node.as_table();
    const std::string category_path =
        "categories[" + std::to_string(category_index) + "]";
    if (category == nullptr) {
      AppendIssue(report, "schema",
                  "config.validator_config.category_not_table",
                  "'categories' items must be tables.", category_path);
      continue;
    }

    if (category->get_as<std::string>("parent_item") == nullptr) {
      AppendIssue(report, "schema",
                  "config.validator_config.missing_parent_item",
                  "'parent_item' must be a string.",
                  category_path + ".parent_item");
    }

    const toml::array* sub_items = category->get_as<toml::array>("sub_items");
    if (sub_items == nullptr) {
      AppendIssue(report, "schema",
                  "config.validator_config.missing_sub_items",
                  "'sub_items' must be an array of strings.",
                  category_path + ".sub_items");
      continue;
    }

    for (std::size_t sub_item_index = 0; sub_item_index < sub_items->size();
         ++sub_item_index) {
      if (!(*sub_items)[sub_item_index].is_string()) {
        AppendIssue(report, "schema",
                    "config.validator_config.sub_item_not_string",
                    "'sub_items' items must be strings.",
                    category_path + ".sub_items[" +
                        std::to_string(sub_item_index) + "]");
      }
    }
  }
}

auto ValidateModifierToml(const toml::table& config_toml,
                          ConfigFileValidationReport& report) -> void {
  if (const auto auto_renewal_view = config_toml["auto_renewal_rules"]) {
    const toml::table* renewal_config = auto_renewal_view.as_table();
    if (renewal_config == nullptr) {
      AppendIssue(report, "schema",
                  "config.modifier_config.auto_renewal_not_table",
                  "'auto_renewal_rules' must be a table.",
                  "auto_renewal_rules");
    } else {
      if (const auto enabled_view = (*renewal_config)["enabled"];
          enabled_view && !enabled_view.is_boolean()) {
        AppendIssue(report, "schema",
                    "config.modifier_config.enabled_not_boolean",
                    "'auto_renewal_rules.enabled' must be a boolean.",
                    "auto_renewal_rules.enabled");
      }

      if (const auto rules_view = (*renewal_config)["rules"]) {
        const toml::array* rules = rules_view.as_array();
        if (rules == nullptr) {
          AppendIssue(report, "schema",
                      "config.modifier_config.rules_not_array",
                      "'auto_renewal_rules.rules' must be an array.",
                      "auto_renewal_rules.rules");
        } else {
          for (std::size_t rule_index = 0; rule_index < rules->size();
               ++rule_index) {
            const toml::table* rule = (*rules)[rule_index].as_table();
            const std::string rule_path =
                "auto_renewal_rules.rules[" + std::to_string(rule_index) + "]";
            if (rule == nullptr) {
              AppendIssue(report, "schema",
                          "config.modifier_config.rule_not_table",
                          "'rules' items must be tables.", rule_path);
              continue;
            }
            if (rule->get_as<std::string>("header_location") == nullptr) {
              AppendIssue(report, "schema",
                          "config.modifier_config.missing_header_location",
                          "'header_location' must be a string.",
                          rule_path + ".header_location");
            }
            const toml::node* amount = rule->get("amount");
            if (amount == nullptr ||
                !(amount->is_integer() || amount->is_floating_point())) {
              AppendIssue(report, "schema",
                          "config.modifier_config.missing_amount",
                          "'amount' must be numeric.",
                          rule_path + ".amount");
            }
            if (rule->get_as<std::string>("description") == nullptr) {
              AppendIssue(report, "schema",
                          "config.modifier_config.missing_description",
                          "'description' must be a string.",
                          rule_path + ".description");
            }
          }
        }
      }
    }
  }

  if (const auto metadata_view = config_toml["metadata_prefixes"]) {
    const toml::array* metadata_prefixes = metadata_view.as_array();
    if (metadata_prefixes == nullptr) {
      AppendIssue(report, "schema",
                  "config.modifier_config.metadata_prefixes_not_array",
                  "'metadata_prefixes' must be an array of strings.",
                  "metadata_prefixes");
    } else {
      for (std::size_t index = 0; index < metadata_prefixes->size(); ++index) {
        if (!(*metadata_prefixes)[index].is_string()) {
          AppendIssue(report, "schema",
                      "config.modifier_config.metadata_prefix_not_string",
                      "'metadata_prefixes' items must be strings.",
                      "metadata_prefixes[" + std::to_string(index) + "]");
        }
      }
    }
  }

  if (const auto display_view = config_toml["display_name_maps"]) {
    const toml::table* display_name_maps = display_view.as_table();
    if (display_name_maps == nullptr) {
      AppendIssue(report, "schema",
                  "config.modifier_config.display_name_maps_not_table",
                  "'display_name_maps' must be a table.",
                  "display_name_maps");
    } else {
      for (const auto& [map_key, map_node] : *display_name_maps) {
        const std::string map_path = "display_name_maps." + std::string(map_key.str());
        const toml::table* lang_map = map_node.as_table();
        if (lang_map == nullptr) {
          AppendIssue(report, "schema",
                      "config.modifier_config.display_name_entry_not_table",
                      "Each display_name_maps entry must be a table.",
                      map_path);
          continue;
        }
        for (const auto& [lang_key, lang_value] : *lang_map) {
          if (!lang_value.is_string()) {
            AppendIssue(report, "schema",
                        "config.modifier_config.display_name_value_not_string",
                        "display_name_maps values must be strings.",
                        map_path + "." + std::string(lang_key.str()));
          }
        }
      }
    }
  }
}

auto FinalizeBundleReport(ConfigBundleValidationReport& report) -> void {
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

auto ReadFileOrIssue(const fs::path& path, ConfigFileValidationReport& report)
    -> std::optional<std::string> {
  try {
    if (!fs::is_regular_file(path)) {
      AppendIssue(report, "parse",
                  "config." + report.source_kind + ".file_not_found",
                  "Failed to open config file: " + path.string());
      return std::nullopt;
    }
    return ReadTextFile(path);
  } catch (const std::exception& error) {
    AppendIssue(report, "parse",
                "config." + report.source_kind + ".read_failed", error.what());
    return std::nullopt;
  }
}

}  // namespace

auto ConfigBundleValidationService::ValidateFromConfigDir(
    const fs::path& config_dir)
    -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport> {
  return ValidateFromFiles(config_dir / "validator_config.toml",
                           config_dir / "modifier_config.toml",
                           config_dir / "export_formats.toml");
}

auto ConfigBundleValidationService::ValidateFromFiles(
    const fs::path& validator_config_path, const fs::path& modifier_config_path,
    const fs::path& export_formats_path)
    -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport> {
  ConfigBundleValidationReport report;
  report.files.push_back(MakeConfigReport(kValidatorSourceKind,
                                          validator_config_path.filename().string(),
                                          validator_config_path));
  report.files.push_back(MakeConfigReport(kModifierSourceKind,
                                          modifier_config_path.filename().string(),
                                          modifier_config_path));
  report.files.push_back(MakeConfigReport("export_formats",
                                          export_formats_path.filename().string(),
                                          export_formats_path));

  const auto validator_text =
      ReadFileOrIssue(validator_config_path, report.files[0]);
  const auto modifier_text =
      ReadFileOrIssue(modifier_config_path, report.files[1]);
  const auto export_text = ReadFileOrIssue(export_formats_path, report.files[2]);

  if (!validator_text.has_value() || !modifier_text.has_value() ||
      !export_text.has_value()) {
    FinalizeBundleReport(report);
    return std::unexpected(report);
  }

  return ValidateFromTexts(*validator_text, *modifier_text, *export_text,
                           validator_config_path, modifier_config_path,
                           export_formats_path);
}

auto ConfigBundleValidationService::ValidateFromTexts(
    std::string_view validator_config_text,
    std::string_view modifier_config_text, std::string_view export_formats_text,
    const fs::path& validator_display_path,
    const fs::path& modifier_display_path,
    const fs::path& export_formats_display_path)
    -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport> {
  ConfigBundleValidationReport report;
  report.files.push_back(MakeConfigReport(kValidatorSourceKind,
                                          validator_display_path.filename().string(),
                                          validator_display_path));
  report.files.push_back(MakeConfigReport(kModifierSourceKind,
                                          modifier_display_path.filename().string(),
                                          modifier_display_path));
  report.files.push_back(MakeConfigReport("export_formats",
                                          export_formats_display_path.filename().string(),
                                          export_formats_display_path));

  const ParsedTomlDocument validator_document =
      ParseTomlText(validator_config_text, validator_display_path,
                    kValidatorSourceKind);
  report.files[0].issues = validator_document.issues;
  if (validator_document.ok) {
    ValidateValidatorToml(validator_document.table, report.files[0]);
  }

  const ParsedTomlDocument modifier_document =
      ParseTomlText(modifier_config_text, modifier_display_path,
                    kModifierSourceKind);
  report.files[1].issues = modifier_document.issues;
  if (modifier_document.ok) {
    ValidateModifierToml(modifier_document.table, report.files[1]);
  }

  const ExportFormatConfigValidationResult export_result =
      ExportFormatConfig::ValidateText(export_formats_text,
                                       export_formats_display_path);
  report.files[2].issues = export_result.issues;
  report.enabled_export_formats = export_result.enabled_formats;
  report.available_export_formats = export_result.available_formats;

  RuntimeConfigBundle runtime_config;
  if (report.files[0].issues.empty()) {
    const auto validator_config =
        RuntimeConfigLoader::LoadValidatorConfig(validator_document.table);
    if (!validator_config) {
      AppendIssue(report.files[0], "business",
                  "config.validator_config.load_failed",
                  FormatError(validator_config.error()));
    } else {
      runtime_config.validator_config = *validator_config;
    }
  }

  if (report.files[1].issues.empty()) {
    const auto modifier_config =
        RuntimeConfigLoader::LoadModifierConfig(modifier_document.table);
    if (!modifier_config) {
      AppendIssue(report.files[1], "business",
                  "config.modifier_config.load_failed",
                  FormatError(modifier_config.error()));
    } else {
      runtime_config.modifier_config = *modifier_config;
    }
  }

  FinalizeBundleReport(report);
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
