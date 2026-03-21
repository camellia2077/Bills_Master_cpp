#ifndef REPORTS_CORE_EXPORT_FORMAT_CONFIG_HPP_
#define REPORTS_CORE_EXPORT_FORMAT_CONFIG_HPP_

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "common/validation_issue.hpp"

struct ExportFormatConfigValidationResult {
  bool ok = false;
  std::filesystem::path path;
  std::vector<std::string> enabled_formats;
  std::vector<std::string> available_formats;
  std::vector<ValidationIssue> issues;
};

class ExportFormatConfig {
 public:
  [[nodiscard]] static auto ValidateFile(
      const std::filesystem::path& config_path)
      -> ExportFormatConfigValidationResult;

  [[nodiscard]] static auto ValidateText(
      std::string_view raw_text, const std::filesystem::path& display_path)
      -> ExportFormatConfigValidationResult;
};

#endif  // REPORTS_CORE_EXPORT_FORMAT_CONFIG_HPP_
