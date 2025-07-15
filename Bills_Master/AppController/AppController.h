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
    AppController();

    /**
     * @brief 处理一个或多个账单文件的验证。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    void handle_validation(const std::string& path);

    /**
     * @brief 处理一个或多个账单文件的修改。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    void handle_modification(const std::string& path);

    /**
     * @brief 处理将一个或多个账单文件导入数据库。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    void handle_import(const std::string& path);

    /**
     * @brief 处理完整的处理工作流（验证 -> 修改 -> 导入）。
     * @param path 单个 .txt 文件或要递归搜索的目录的路径。
     */
    void handle_full_workflow(const std::string& path);

    /**
     * @brief 处理报告的导出。
     * @param type 导出类型 ("year", "month", "all")。
     * @param value 导出的具体值（例如，年份或月份字符串），对于 "all" 类型则忽略。
     */
    void handle_export(const std::string& type, const std::string& value = "");

    /**
     * @brief 显示应用程序的版本信息。
     */
    void display_version();
};

#endif // APP_CONTROLLER_H