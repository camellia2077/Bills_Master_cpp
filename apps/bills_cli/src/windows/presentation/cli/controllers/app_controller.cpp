// windows/presentation/cli/controllers/app_controller.cpp
#include "app_controller.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

#include "common/cli_version.hpp"
#include "common/common_utils.hpp"

namespace terminal = common::terminal;
#include "export/export_controller.hpp"
#include <toml++/toml.hpp>
#include "workflow/workflow_controller.hpp"
#if BILLS_CORE_MODULES_ENABLED
import bill.core.common.version;
#else
#include "common/version.hpp"
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
namespace {
constexpr const char* kExportFormatConfigName = "export_formats.toml";
constexpr const char* kLegacyPipelineDeprecatedSince = "2026-03-05";
constexpr const char* kLegacyPipelineRemovalTarget = "2026-06-30";

auto is_builtin_export_format(std::string format_name) -> bool {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  if (format_name == "json" || format_name == "md" ||
      format_name == "markdown") {
    return true;
  }
#if BILLS_FMT_RST_ENABLED
  if (format_name == "rst") {
    return true;
  }
#endif
#if BILLS_FMT_TEX_ENABLED
  if (format_name == "tex" || format_name == "latex") {
    return true;
  }
#endif
  return false;
}

auto GetExecutableDirectory() -> fs::path {
#ifdef _WIN32
  wchar_t path[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(path).parent_path();
#else
  return fs::current_path();
#endif
}

auto JoinFormats(const std::set<std::string>& formats) -> std::string {
  if (formats.empty()) {
    return "(none)";
  }

  std::ostringstream out;
  bool first = true;
  for (const auto& format : formats) {
    if (!first) {
      out << ", ";
    }
    out << format;
    first = false;
  }
  return out.str();
}
}  // namespace

AppController::AppController(std::string db_path,
                             const std::string& config_path,
                             std::string modified_output_dir)
    : m_db_path(std::move(db_path)),
      m_config_path(config_path),
      m_modified_output_dir(std::move(modified_output_dir)) {
  std::error_code make_output_error;
  fs::create_directories("output", make_output_error);
  if (make_output_error) {
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "Failed to ensure output directory exists: "
              << make_output_error.message() << std::endl;
  }

  fs::path exe_dir = GetExecutableDirectory();

  fs::path config_path_resolved = config_path;
  if (config_path_resolved.is_relative()) {
    config_path_resolved = exe_dir / config_path_resolved;
  }
  m_config_path = config_path_resolved.string();
  m_enabled_formats = load_enabled_formats(m_config_path);

  m_export_base_dir = "output/exported_files";
  m_format_folder_names = {{"md", "Markdown_bills"},
                           {"json", "JSON_bills"},
                           {"tex", "LaTeX_bills"},
                           {"rst", "reST_bills"}};
  m_workflow_controller =
      std::make_unique<WorkflowController>(m_config_path, m_modified_output_dir);
  m_export_controller = std::make_unique<ExportController>(
      m_db_path, m_export_base_dir, m_format_folder_names);
}

AppController::~AppController() = default;

auto AppController::load_enabled_formats(const std::string& config_path)
    -> std::set<std::string> {
  std::set<std::string> enabled_formats;
  const fs::path config_file = fs::path(config_path) / kExportFormatConfigName;

  try {
    if (!fs::is_regular_file(config_file)) {
      enabled_formats.insert("md");
      std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
                << "Export format config not found: " << config_file.string()
                << ". Falling back to default format set: md." << std::endl;
      return enabled_formats;
    }

    const toml::table config_toml = toml::parse_file(config_file.string());
    const toml::array* format_list =
        config_toml["enabled_formats"].as_array();

    if (format_list == nullptr) {
      throw std::runtime_error(
          "'enabled_formats' must be a TOML array in " + config_file.string());
    }

    for (const auto& item : *format_list) {
      const auto value = item.value<std::string>();
      if (!value.has_value()) {
        continue;
      }
      const std::string normalized = normalize_format(*value);
      if (!normalized.empty()) {
        enabled_formats.insert(normalized);
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "Failed to load export formats from " << config_file.string()
              << ". Reason: " << ex.what()
              << ". Falling back to default format set: md." << std::endl;
    enabled_formats.clear();
    enabled_formats.insert("md");
  }

  if (enabled_formats.empty()) {
    enabled_formats.insert("md");
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "No formats configured in " << config_file.string()
              << ". Falling back to default format set: md." << std::endl;
  }
  return enabled_formats;
}

auto AppController::normalize_format(std::string format_name) const
    -> std::string {
  std::transform(format_name.begin(), format_name.end(), format_name.begin(),
                 [](unsigned char ch) -> char {
                   return static_cast<char>(std::tolower(ch));
                 });
  return format_name;
}

auto AppController::normalize_export_pipeline(std::string pipeline_name) const
    -> std::string {
  std::transform(pipeline_name.begin(), pipeline_name.end(),
                 pipeline_name.begin(), [](unsigned char ch) -> char {
                   return static_cast<char>(std::tolower(ch));
                 });
  std::ranges::replace(pipeline_name, '_', '-');
  if (pipeline_name.empty()) {
    return "model-first";
  }
  if (pipeline_name == "jsonfirst") {
    return "json-first";
  }
  if (pipeline_name == "modelfirst") {
    return "model-first";
  }
  return pipeline_name;
}

auto AppController::is_export_format_available(
    const std::string& type, const std::vector<std::string>& values,
    const std::string& format_str,
    const std::string& export_pipeline) const -> bool {
  (void)type;
  (void)values;
  (void)export_pipeline;
  const std::string format = normalize_format(format_str);
  if (m_enabled_formats.count(format) == 0U) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Format '" << format
              << "' is not enabled. Enabled formats: "
              << JoinFormats(m_enabled_formats)
              << ". Please update config/" << kExportFormatConfigName << "."
              << std::endl;
    return false;
  }
  if (!is_builtin_export_format(format)) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Format '" << format
              << "' is enabled in config but not available in this build."
              << std::endl;
    return false;
  }

  return true;
}

// ... handle_validation, handle_convert 等其他方法保持不变 ...
auto AppController::handle_validation(const std::string& path) -> bool {
  return m_workflow_controller->handle_validation(path);
}

auto AppController::handle_convert(const std::string& path) -> bool {
  return m_workflow_controller->handle_convert(path);
}

auto AppController::handle_ingest(const std::string& path, bool write_json)
    -> bool {
  return m_workflow_controller->handle_ingest(path, m_db_path, write_json);
}

auto AppController::handle_import(const std::string& path) -> bool {
  return m_workflow_controller->handle_import(path, m_db_path);
}

auto AppController::handle_full_workflow(const std::string& path) -> bool {
  return m_workflow_controller->handle_full_workflow(path, m_db_path);
}

auto AppController::handle_export(const std::string& type,
                                  const std::vector<std::string>& values,
                                  const std::string& format_str,
                                  const std::string& export_pipeline) -> bool {
  const std::string normalized_format = normalize_format(format_str);
  const std::string normalized_pipeline =
      normalize_export_pipeline(export_pipeline);
  if (normalized_pipeline != "legacy" &&
      normalized_pipeline != "model-first" &&
      normalized_pipeline != "json-first") {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Unknown value for --export-pipeline: '" << export_pipeline
              << "'. Use 'legacy', 'model-first', or 'json-first'."
              << std::endl;
    return false;
  }

  if (!is_export_format_available(type, values, normalized_format,
                                  normalized_pipeline)) {
    return false;
  }

  if (normalized_pipeline == "legacy") {
    static bool legacy_warning_emitted = false;
    if (!legacy_warning_emitted) {
      std::cerr
          << terminal::kYellow << "Warning: " << terminal::kReset
          << "Export pipeline 'legacy' is transitional and deprecated since "
          << kLegacyPipelineDeprecatedSince << ". Planned removal target is "
          << kLegacyPipelineRemovalTarget
          << ". Prefer 'model-first' (default) or 'json-first'." << std::endl;
      legacy_warning_emitted = true;
    }
  }

  return m_export_controller->handle_export(type, values, normalized_format,
                                            normalized_pipeline);
}

void AppController::display_version() {
#if BILLS_CORE_MODULES_ENABLED
  constexpr auto kCoreVersion = bills::core::modules::common_version::kVersion;
  constexpr auto kCoreLastUpdated =
      bills::core::modules::common_version::kLastUpdated;
#else
  constexpr auto kCoreVersion = bills::core::version::kVersion;
  constexpr auto kCoreLastUpdated = bills::core::version::kLastUpdated;
#endif
  std::cout << "Core Version: " << kCoreVersion << std::endl;
  std::cout << "Core Last Updated: " << kCoreLastUpdated << std::endl;
  std::cout << "CLI Version: " << bills::cli::version::kVersion << std::endl;
  std::cout << "CLI Last Updated: " << bills::cli::version::kLastUpdated
            << std::endl;
}

