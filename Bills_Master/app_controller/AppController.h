#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "ProcessStats.h"
#include <string>

/**
 * @class AppController
 * @brief 作为应用程序的中央控制器。
 *
 * 封装了验证、修改、数据库导入和查询等核心业务逻辑。
 * 旨在被不同的用户界面（例如，交互式菜单或命令行工具）调用。
 */
class AppController {
public:
   // 构造函数，可以接受配置路径，或使用默认值
    explicit AppController(const std::string& db_path = "bills.sqlite3", const std::string& plugin_path = "plugins");

    /**
     * @brief 处理一个或多个账单文件的验证。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    bool handle_validation(const std::string& path);

    /**
     * @brief 处理一个或多个账单文件的修改。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    bool handle_modification(const std::string& path);

    /**
     * @brief 处理将一个或多个账单文件导入数据库。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    bool handle_import(const std::string& path);

    /**
     * @brief 处理完整的处理工作流（验证 -> 修改 -> 导入）。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    bool handle_full_workflow(const std::string& path);

    /**
     * @brief 处理报告的导出。
     * @param type 导出类型 ("year", "month", "all")。
     * @param value 导出的具体值（例如，年份或月份字符串），对于 "all" 类型则忽略。
     * @param format_str 导出的格式 ("md", "tex", "typ")，默认为 "md"。
     */
    bool handle_export(const std::string& type, const std::string& value = "", const std::string& format_str = "md");

    /**
     * @brief 显示应用程序的版本信息。
     */
    void display_version();

private:
    std::string m_db_path;
    std::string m_plugin_path;
};

#endif // APP_CONTROLLER_H