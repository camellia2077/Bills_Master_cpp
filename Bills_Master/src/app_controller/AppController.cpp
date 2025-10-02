// app_controller/AppController.cpp
#include "AppController.hpp"
#include "WorkflowController.hpp"
#include "ExportController.hpp"
#include "common/version.hpp"
#include "common/common_utils.hpp"
#include <iostream>

// 引入以下头文件以支持路径操作和Windows API
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// [新增] 辅助函数，用于获取可执行文件所在的目录
// 我们将其放在一个匿名命名空间中，使其仅在此文件内可见
namespace {
    fs::path get_executable_directory() {
#ifdef _WIN32
        wchar_t path[MAX_PATH] = {0};
        // GetModuleFileNameW 是一个Windows函数，用于获取当前进程（.exe）的完整路径
        GetModuleFileNameW(NULL, path, MAX_PATH);
        // fs::path(path) 将宽字符路径转换为C++的路径对象
        // .parent_path() 则获取包含该文件的目录
        return fs::path(path).parent_path();
#else
        // 为其他操作系统（如Linux）提供一个备用方案，尽管当前环境是Windows
        return fs::current_path();
#endif
    }
}

// 在构造函数中恢复对插件文件列表的初始化
AppController::AppController(const std::string& db_path, 
                             const std::string& config_path, 
                             const std::string& modified_output_dir)
    : m_db_path(db_path),
      m_config_path(config_path),
      m_modified_output_dir(modified_output_dir) {
    
    // [修改] 动态构建插件路径
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

    m_export_base_dir = "exported_files";
    m_format_folder_names = {
        {"md", "Markdown_bills"},
        {"tex", "LaTeX_bills"},
        {"rst", "reST_bills"},
        {"typ", "Typst_bills"}
    };
}

// 2. 将具体实现委托给 WorkflowController
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
    return workflow.handle_import(path);
}

bool AppController::handle_full_workflow(const std::string& path) {
    WorkflowController workflow(m_config_path, m_modified_output_dir);
    return workflow.handle_full_workflow(path);
}

// 3. 将具体实现委托给 ExportController
bool AppController::handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str) {
    ExportController exporter(m_db_path, m_plugin_files, m_export_base_dir, m_format_folder_names);
    return exporter.handle_export(type, values, format_str);
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}