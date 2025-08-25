
#include "AppController.h"
#include "WorkflowController.h"
#include "ExportController.h"
#include "common/version.h"
#include "common/common_utils.h"
#include <iostream>

// 在构造函数中恢复对插件文件列表的初始化
AppController::AppController(const std::string& db_path, 
                             const std::string& config_path, 
                             const std::string& modified_output_dir)
    : m_db_path(db_path),
      m_config_path(config_path),
      m_modified_output_dir(modified_output_dir) {
    
    // ✅ 恢复这部分被遗漏的逻辑
    m_plugin_files = {
        "plugins/md_month_formatter.dll",
        "plugins/rst_month_formatter.dll",
        "plugins/tex_month_formatter.dll",
        "plugins/typ_month_formatter.dll",
        
        "plugins/md_year_formatter.dll",
        "plugins/rst_year_formatter.dll",
        "plugins/tex_year_formatter.dll",
        "plugins/typ_year_formatter.dll"
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

// display_version 保持不变
void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}