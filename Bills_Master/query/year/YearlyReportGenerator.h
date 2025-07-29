// YearlyReportGenerator.h
#ifndef YEARLY_REPORT_GENERATOR_H
#define YEARLY_REPORT_GENERATOR_H

#include <string>
#include <vector> // 1. 新增: 为了接收DLL路径列表
#include <sqlite3.h>
#include "query/year/year_query/YearlyDataReader.h"
// 2. 替换: 从旧的静态工厂替换为新的插件管理器
#include "query/year/year_format/YearPluginLoader.h" 

/**
 * @class YearlyReportGenerator
 * @brief 一个通过动态加载插件来简化报告生成过程的封装类。
 */
class YearlyReportGenerator {
public:
    /**
     * @brief 构造函数，通过扫描插件目录来初始化。
     * @param db_connection SQLite数据库连接。
     * @param plugin_directory_path 指向包含插件动态库的目录的路径。
     */
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path);

    /**
     * @brief 构造函数，通过加载指定的插件文件列表来初始化。
     * @param db_connection SQLite数据库连接。
     * @param plugin_file_paths 包含一个或多个插件DLL完整路径的向量。
     */
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths);


    /**
     * @brief 主接口，用于生成报告。
     * @param year 要查询的年份。
     * @param format 所需的输出格式 (默认为Markdown)。
     * @return 包含所请求格式报告的字符串。
     */
    // The signature is updated to take a string, removing the enum.
    std::string generate(int year, const std::string& format_name);

private:
    YearlyDataReader m_reader; // 4. 保留: 数据读取器仍然需要 
    YearPluginLoader m_plugin_manager; // 3. 替换: 使用新的插件管理器作为成员
};

#endif // YEARLY_REPORT_GENERATOR_H