// controllers/AppController.cpp
#include "AppController.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>

#include "common/common_utils.hpp"
#include "common/version.hpp"
#include "export/ExportController.hpp"
#include "nlohmann/json.hpp"
#include "platform/windows/infrastructure/file_handler/FileHandler.hpp"
#include "workflow/WorkflowController.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
using Json = nlohmann::json;

namespace {
constexpr const char* kExportFormatConfigName = "Export_Formats.json";
constexpr const char* kMonthPluginSuffix = "_month_formatter.dll";
constexpr const char* kYearPluginSuffix = "_year_formatter.dll";

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
  // [新增] 使用 FileHandler 创建 output 文件夹
  FileHandler::create_directories("output");

  fs::path plugin_dir = GetExecutableDirectory() / "plugins";
  fs::path exe_dir = GetExecutableDirectory();

  fs::path config_path_resolved = config_path;
  if (config_path_resolved.is_relative()) {
    config_path_resolved = exe_dir / config_path_resolved;
  }
  m_config_path = config_path_resolved.string();
  m_enabled_formats = load_enabled_formats(m_config_path);
  for (const auto& format : m_enabled_formats) {
    const fs::path month_plugin = plugin_dir / (format + kMonthPluginSuffix);
    if (fs::is_regular_file(month_plugin)) {
      m_plugin_files.push_back(month_plugin.string());
      m_month_formats_available.insert(format);
    } else {
      std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR
                << "Month formatter plugin not found for format '" << format
                << "': " << month_plugin.string() << std::endl;
    }

    const fs::path year_plugin = plugin_dir / (format + kYearPluginSuffix);
    if (fs::is_regular_file(year_plugin)) {
      m_plugin_files.push_back(year_plugin.string());
      m_year_formats_available.insert(format);
    } else {
      std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR
                << "Year formatter plugin not found for format '" << format
                << "': " << year_plugin.string() << std::endl;
    }
  }

  m_export_base_dir = "output/exported_files";
  m_format_folder_names = {{"md", "Markdown_bills"},
                           {"tex", "LaTeX_bills"},
                           {"rst", "reST_bills"},
                           {"typ", "Typst_bills"}};
}

auto AppController::load_enabled_formats(const std::string& config_path)
    -> std::set<std::string> {
  std::set<std::string> enabled_formats;
  const fs::path config_file = fs::path(config_path) / kExportFormatConfigName;

  try {
    if (!fs::is_regular_file(config_file)) {
      enabled_formats.insert("md");
      std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR
                << "Export format config not found: " << config_file.string()
                << ". Falling back to default format set: md." << std::endl;
      return enabled_formats;
    }

    std::ifstream input(config_file);
    if (!input) {
      throw std::runtime_error("Unable to open file.");
    }

    Json config_json;
    input >> config_json;
    const Json format_list =
        config_json.value("enabled_formats", Json::array());

    if (!format_list.is_array()) {
      throw std::runtime_error(
          "'enabled_formats' must be a JSON array in " + config_file.string());
    }

    for (const auto& item : format_list) {
      if (!item.is_string()) {
        continue;
      }
      const std::string normalized = normalize_format(item.get<std::string>());
      if (!normalized.empty()) {
        enabled_formats.insert(normalized);
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR
              << "Failed to load export formats from " << config_file.string()
              << ". Reason: " << ex.what()
              << ". Falling back to default format set: md." << std::endl;
    enabled_formats.clear();
    enabled_formats.insert("md");
  }

  if (enabled_formats.empty()) {
    enabled_formats.insert("md");
    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR
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

auto AppController::infer_export_requirements(
    const std::string& type, const std::vector<std::string>& values) const
    -> std::pair<bool, bool> {
  const std::string type_normalized = normalize_format(type);
  bool need_month = false;
  bool need_year = false;

  if (type_normalized == "all") {
    need_month = true;
    need_year = true;
  } else if (type_normalized == "all_months" || type_normalized == "month") {
    need_month = true;
  } else if (type_normalized == "all_years" || type_normalized == "year") {
    need_year = true;
  } else if (type_normalized == "date") {
    if (values.size() == 1U && values[0].size() == 4U) {
      need_year = true;
    } else {
      need_month = true;
    }
  }

  return {need_month, need_year};
}

auto AppController::is_export_format_available(
    const std::string& type, const std::vector<std::string>& values,
    const std::string& format_str) const -> bool {
  const std::string format = normalize_format(format_str);
  if (m_enabled_formats.count(format) == 0U) {
    std::cerr << RED_COLOR << "Error: " << RESET_COLOR
              << "Format '" << format
              << "' is not enabled. Enabled formats: "
              << JoinFormats(m_enabled_formats)
              << ". Please update config/" << kExportFormatConfigName << "."
              << std::endl;
    return false;
  }

  const auto [need_month, need_year] = infer_export_requirements(type, values);
  if (need_month && m_month_formats_available.count(format) == 0U) {
    std::cerr << RED_COLOR << "Error: " << RESET_COLOR
              << "Month formatter plugin for format '" << format
              << "' is not available in runtime plugins directory."
              << std::endl;
    return false;
  }
  if (need_year && m_year_formats_available.count(format) == 0U) {
    std::cerr << RED_COLOR << "Error: " << RESET_COLOR
              << "Year formatter plugin for format '" << format
              << "' is not available in runtime plugins directory."
              << std::endl;
    return false;
  }

  return true;
}

// ... handle_validation, handle_modification 等其他方法保持不变 ...
auto AppController::handle_validation(const std::string& path) -> bool {
  WorkflowController workflow(m_config_path, m_modified_output_dir);
  return workflow.handle_validation(path);
}

auto AppController::handle_modification(const std::string& path) -> bool {
  return handle_convert(path);
}

auto AppController::handle_convert(const std::string& path) -> bool {
  WorkflowController workflow(m_config_path, m_modified_output_dir);
  return workflow.handle_convert(path);
}

auto AppController::handle_ingest(const std::string& path, bool write_json)
    -> bool {
  WorkflowController workflow(m_config_path, m_modified_output_dir);
  return workflow.handle_ingest(path, m_db_path, write_json);
}

auto AppController::handle_import(const std::string& path) -> bool {
  WorkflowController workflow(m_config_path, m_modified_output_dir);
  return workflow.handle_import(path, m_db_path);
}

auto AppController::handle_full_workflow(const std::string& path) -> bool {
  WorkflowController workflow(m_config_path, m_modified_output_dir);
  return workflow.handle_full_workflow(path, m_db_path);
}

auto AppController::handle_export(const std::string& type,
                                  const std::vector<std::string>& values,
                                  const std::string& format_str) -> bool {
  const std::string normalized_format = normalize_format(format_str);
  if (!is_export_format_available(type, values, normalized_format)) {
    return false;
  }

  ExportController exporter(m_db_path, m_plugin_files, m_export_base_dir,
                            m_format_folder_names);
  return exporter.handle_export(type, values, normalized_format);
}

void AppController::display_version() {
  std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
  std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}

