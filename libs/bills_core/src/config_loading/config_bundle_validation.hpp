#ifndef CONFIG_LOADING_CONFIG_BUNDLE_VALIDATION_HPP_
#define CONFIG_LOADING_CONFIG_BUNDLE_VALIDATION_HPP_

#include <expected>
#include <filesystem>
#include <string_view>
#include <vector>

#include "config_loading/runtime_config_loader.hpp"
#include "reports/core/export_format_config.hpp"

struct ConfigFileValidationReport {
  std::string source_kind;
  std::string file_name;
  std::filesystem::path path;
  bool ok = false;
  std::vector<ValidationIssue> issues;
};

struct ConfigBundleValidationReport {
  bool ok = false;
  std::size_t processed = 0;
  std::size_t success = 0;
  std::size_t failure = 0;
  std::vector<ConfigFileValidationReport> files;
  std::vector<std::string> enabled_export_formats;
  std::vector<std::string> available_export_formats;
};

struct ValidatedConfigBundle {
  RuntimeConfigBundle runtime_config;
  std::vector<std::string> enabled_export_formats;
  std::vector<std::string> available_export_formats;
  ConfigBundleValidationReport report;
};

class ConfigBundleValidationService {
 public:
  [[nodiscard]] static auto ValidateFromConfigDir(
      const std::filesystem::path& config_dir)
      -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport>;

  [[nodiscard]] static auto ValidateFromFiles(
      const std::filesystem::path& validator_config_path,
      const std::filesystem::path& modifier_config_path,
      const std::filesystem::path& export_formats_path)
      -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport>;

  [[nodiscard]] static auto ValidateFromTexts(
      std::string_view validator_config_text,
      std::string_view modifier_config_text,
      std::string_view export_formats_text,
      const std::filesystem::path& validator_display_path =
          "validator_config.toml",
      const std::filesystem::path& modifier_display_path =
          "modifier_config.toml",
      const std::filesystem::path& export_formats_display_path =
          "export_formats.toml")
      -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport>;
};

#endif  // CONFIG_LOADING_CONFIG_BUNDLE_VALIDATION_HPP_
