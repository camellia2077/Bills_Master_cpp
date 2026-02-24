// app_controller/AppController.cpp
#include "AppController.hpp"

#include <filesystem>
#include <iostream>
#include <utility>

#include "common/common_utils.hpp"
#include "common/version.hpp"
#include "export/ExportController.hpp"
#include "file_handler/FileHandler.hpp"
#include "workflow/WorkflowController.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
auto GetExecutableDirectory() -> fs::path {
#ifdef _WIN32
  wchar_t path[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(path).parent_path();
#else
  return fs::current_path();
#endif
}
}  // namespace

AppController::AppController(std::string db_path,
                             const std::string& config_path,
                             std::string modified_output_dir)
    : m_db_path(std::move(db_path)),
      m_config_path(config_path),
      m_modified_output_dir(std::move(modified_output_dir)) {
  // [新增] 使用 FileHandler 创建 output 文件夹
  FileHandler file_handler;
  FileHandler::create_directories("output");

  fs::path plugin_dir = GetExecutableDirectory() / "plugins";
  fs::path exe_dir = GetExecutableDirectory();

  fs::path config_path_resolved = config_path;
  if (config_path_resolved.is_relative()) {
    config_path_resolved = exe_dir / config_path_resolved;
  }
  m_config_path = config_path_resolved.string();

  m_plugin_files = {(plugin_dir / "md_month_formatter.dll").string(),
                    (plugin_dir / "rst_month_formatter.dll").string(),
                    (plugin_dir / "tex_month_formatter.dll").string(),
                    (plugin_dir / "typ_month_formatter.dll").string(),

                    (plugin_dir / "md_year_formatter.dll").string(),
                    (plugin_dir / "rst_year_formatter.dll").string(),
                    (plugin_dir / "tex_year_formatter.dll").string(),
                    (plugin_dir / "typ_year_formatter.dll").string()};

  m_export_base_dir = "output/exported_files";
  m_format_folder_names = {{"md", "Markdown_bills"},
                           {"tex", "LaTeX_bills"},
                           {"rst", "reST_bills"},
                           {"typ", "Typst_bills"}};
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
  ExportController exporter(m_db_path, m_plugin_files, m_export_base_dir,
                            m_format_folder_names);
  return exporter.handle_export(type, values, format_str);
}

void AppController::display_version() {
  std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
  std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}
