// app_controller/AppController.cpp
#include "AppController.hpp"
#include "workflow/WorkflowController.hpp"
#include "export/ExportController.hpp"

#include "file_handler/FileHandler.hpp"
#include "common/version.hpp"
#include "common/common_utils.hpp"
#include <iostream>

#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
    fs::path get_executable_directory() {
#ifdef _WIN32
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return fs::path(path).parent_path();
#else
        return fs::current_path();
#endif
    }
}

AppController::AppController(const std::string& db_path, 
                             const std::string& config_path, 
                             const std::string& modified_output_dir)
    : m_db_path(db_path),
      m_config_path(config_path),
      m_modified_output_dir(modified_output_dir) {
    
    // [新增] 使用 FileHandler 创建 output 文件夹
    FileHandler file_handler;
    file_handler.create_directories("output");

    fs::path plugin_dir = get_executable_directory() / "plugins";

    m_plugin_files = {
        (plugin_dir / "md_month_formatter.dll").string(),
        (plugin_dir / "rst_month_formatter.dll").string(),
        (plugin_dir / "tex_month_formatter.dll").string(),
        (plugin_dir / "typ_month_formatter.dll").string(),
        
        (plugin_dir / "md_year_formatter.dll").string(),
        (plugin_dir / "rst_year_formatter.dll").string(),
        (plugin_dir / "tex_year_formatter.dll").string(),
        (plugin_dir / "typ_year_formatter.dll").string()
    };

    m_export_base_dir = "output/exported_files";
    m_format_folder_names = {
        {"md", "Markdown_bills"},
        {"tex", "LaTeX_bills"},
        {"rst", "reST_bills"},
        {"typ", "Typst_bills"}
    };
}

// ... handle_validation, handle_modification 等其他方法保持不变 ...
bool AppController::handle_validation(const std::string& path) {
    WorkflowController workflow(m_config_path, m_modified_output_dir);
    return workflow.handle_validation(path);
}

bool AppController::handle_modification(const std::string& path) {
    WorkflowController workflow(m_config_path, m_modified_output_dir);
    return workflow.handle_modification(path);
}

bool AppController::handle_import(const std::string& path) {
    WorkflowController workflow(m_config_path, m_modified_output_dir);
    return workflow.handle_import(path, m_db_path);
}

bool AppController::handle_full_workflow(const std::string& path) {
    WorkflowController workflow(m_config_path, m_modified_output_dir);
    return workflow.handle_full_workflow(path, m_db_path);
}

bool AppController::handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str) {
    ExportController exporter(m_db_path, m_plugin_files, m_export_base_dir, m_format_folder_names);
    return exporter.handle_export(type, values, format_str);
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}