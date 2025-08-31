#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <string>
#include <vector>
#include <map>

// 移除 #include "ProcessStats.hpp"

class AppController {
public:
   explicit AppController(const std::string& db_path = "bills.sqlite3", 
                          const std::string& config_path = "./config",
                          const std::string& modified_output_dir = "txt2josn");

    // 函数签名保持不变，但实现会变得非常简单
    bool handle_validation(const std::string& path);
    bool handle_modification(const std::string& path);
    bool handle_import(const std::string& path);
    bool handle_full_workflow(const std::string& path);
    bool handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str = "md");

    void display_version();

private:
    // 保留所有配置项，因为它们需要被传递给子控制器
    std::string m_db_path;
    std::string m_config_path;
    std::string m_modified_output_dir;
    std::vector<std::string> m_plugin_files;
    std::string m_export_base_dir;
    std::map<std::string, std::string> m_format_folder_names;
};

#endif // APP_CONTROLLER_H