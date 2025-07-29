#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "ProcessStats.h"
#include <string>
#include <vector>
#include <map> // [修改] 新增头文件以支持 std::map

/**
 * @class AppController
 * @brief Acts as the central controller for the application.
 *
 * Encapsulates core business logic for validation, modification, database import, and querying.
 * Designed to be called by different user interfaces (e.g., interactive menu or command-line tool).
 */
class AppController {
public:
   explicit AppController(const std::string& db_path = "bills.sqlite3", 
                          const std::string& config_path = "./config",// 预处理的json文件夹
                          const std::string& modified_output_dir = "txt_raw"); // 处理后存储的txt的文件夹

    /**
     * @brief Handles validation for one or more bill files.
     * @param path Path to a single .txt file or a directory to search recursively.
     */
    bool handle_validation(const std::string& path);

    /**
     * @brief Handles modification for one or more bill files.
     * @param path Path to a single .txt file or a directory to search recursively.
     */
    bool handle_modification(const std::string& path);

    /**
     * @brief Handles importing one or more bill files into the database.
     * @param path Path to a single .txt file or a directory to search recursively.
     */
    bool handle_import(const std::string& path);

    /**
     * @brief Handles the full processing workflow (Validate -> Modify -> Import).
     * @param path Path to a single .txt file or a directory to search recursively.
     */
    bool handle_full_workflow(const std::string& path);

    /**
     * @brief Handles the exporting of reports based on type and values.
     * @param type The type of export ("all", "all_months", "all_years", "year", "month", "date").
     * @param values A vector of string values (e.g., year, month, start/end dates).
     * @param format_str The format to export in ("md", "tex", etc.).
     */
    bool handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str = "md");

    /**
     * @brief Displays the application's version information.
     */
    void display_version();

private:
    std::string m_db_path;
    std::string m_config_path; // 用于存放预处理json config文件夹的路径
    std::string m_modified_output_dir; // <-- 新增：用于存放预处理后的文件目录
    std::vector<std::string> m_plugin_files;
    std::string m_export_base_dir;// 成员用于保存导出目录配置
    std::map<std::string, std::string> m_format_folder_names;
};

#endif // APP_CONTROLLER_H