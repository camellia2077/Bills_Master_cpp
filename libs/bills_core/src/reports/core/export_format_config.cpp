#include "reports/core/export_format_config.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include <toml++/toml.hpp>

#include "reports/core/standard_report_renderer_registry.hpp"

namespace {
namespace fs = std::filesystem;

constexpr const char* kSourceKind = "export_formats";

auto ReadTextFile(const fs::path& config_path) -> std::string {
  std::ifstream input(config_path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open config file: " + config_path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto BuildFieldPath(std::size_t index) -> std::string {
  return "enabled_formats[" + std::to_string(index) + "]";
}

auto AppendIssue(ExportFormatConfigValidationResult& result, std::string stage,
                 std::string code, std::string message, std::string field_path = {},
                 int line = 0, int column = 0) -> void {
  result.issues.push_back(MakeValidationIssue(
      kSourceKind, std::move(stage), std::move(code), std::move(message),
      result.path, line, column, std::move(field_path)));
}

}  // namespace

auto ExportFormatConfig::ValidateFile(const fs::path& config_path)
    -> ExportFormatConfigValidationResult {
  ExportFormatConfigValidationResult result;
  result.path = config_path;
  result.available_formats =
      StandardReportRendererRegistry::ListAvailableFormats();
  try {
    if (!fs::is_regular_file(config_path)) {
      AppendIssue(result, "parse", "config.export_formats.file_not_found",
                  "Failed to open config file: " + config_path.string());
      return result;
    }
    return ValidateText(ReadTextFile(config_path), config_path);
  } catch (const std::exception& error) {
    AppendIssue(result, "parse", "config.export_formats.read_failed",
                error.what());
    return result;
  }
}

auto ExportFormatConfig::ValidateText(std::string_view raw_text,
                                      const fs::path& display_path)
    -> ExportFormatConfigValidationResult {
  ExportFormatConfigValidationResult result;
  result.path = display_path;
  result.available_formats =
      StandardReportRendererRegistry::ListAvailableFormats();

  toml::table config_toml;
  try {
    config_toml = toml::parse(raw_text, display_path.string());
  } catch (const toml::parse_error& error) {
    AppendIssue(result, "parse", "config.export_formats.parse_error",
                error.what(), {}, static_cast<int>(error.source().begin.line),
                static_cast<int>(error.source().begin.column));
    return result;
  } catch (const std::exception& error) {
    AppendIssue(result, "parse", "config.export_formats.parse_failed",
                error.what());
    return result;
  }

  const toml::array* format_list = config_toml["enabled_formats"].as_array();
  if (format_list == nullptr) {
    AppendIssue(result, "schema",
                "config.export_formats.enabled_formats_not_array",
                "'enabled_formats' must be a non-empty array.",
                "enabled_formats");
    return result;
  }

  std::set<std::string> deduped_formats;
  for (std::size_t index = 0; index < format_list->size(); ++index) {
    const toml::node& item = (*format_list)[index];
    const auto value = item.value<std::string>();
    if (!value.has_value()) {
      AppendIssue(result, "schema", "config.export_formats.item_not_string",
                  "'enabled_formats' items must be non-empty strings.",
                  BuildFieldPath(index));
      continue;
    }

    const std::string raw_value = *value;
    if (raw_value.empty()) {
      AppendIssue(result, "schema", "config.export_formats.item_empty",
                  "'enabled_formats' items must be non-empty strings.",
                  BuildFieldPath(index));
      continue;
    }

    const std::string normalized =
        StandardReportRendererRegistry::NormalizeFormat(raw_value);
    if (normalized.empty()) {
      AppendIssue(result, "business",
                  "config.export_formats.unknown_format",
                  "Format '" + raw_value + "' is not recognized.",
                  BuildFieldPath(index));
      continue;
    }

    if (!StandardReportRendererRegistry::IsFormatAvailable(normalized)) {
      AppendIssue(result, "business",
                  "config.export_formats.unavailable_in_build",
                  "Format '" + normalized +
                      "' is enabled in config but not available in this build.",
                  BuildFieldPath(index));
      continue;
    }

    if (deduped_formats.insert(normalized).second) {
      result.enabled_formats.push_back(normalized);
    }
  }

  if (result.issues.empty() && result.enabled_formats.empty()) {
    AppendIssue(result, "schema",
                "config.export_formats.no_enabled_formats",
                "'enabled_formats' must contain at least one usable format.",
                "enabled_formats");
  }

  result.ok = result.issues.empty();
  return result;
}
