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
   explicit AppController(const std::string& db_path = "bills.sqlite3");

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
     * @brief Handles the exporting of reports.
     * @param type The type of export ("year", "month", "all").
     * @param value The specific value to export (e.g., year or month string), ignored for "all".
     * @param format_str The format to export in ("md", "tex", "typ"), defaults to "md".
     */
    bool handle_export(const std::string& type, const std::string& value = "", const std::string& format_str = "md");

    /**
     * @brief Displays the application's version information.
     */
    void display_version();

private:
    std::string m_db_path;
    std::vector<std::string> m_plugin_files;

    // [修改] 新增成员用于保存导出目录配置
    std::string m_export_base_dir;
    std::map<std::string, std::string> m_format_folder_names;
};

#endif // APP_CONTROLLER_H