// query/components/yearly_report/YearlyReportGenerator.h
#ifndef YEARLY_REPORT_GENERATOR_H
#define YEARLY_REPORT_GENERATOR_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "YearlyDataReader.h"
#include "query/plugins/common/PluginLoader.h" // 1. 包含新的通用加载器
#include "query/plugins/year_formatters/IYearlyReportFormatter.h" // 2. 包含接口

class YearlyReportGenerator {
public:
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path);
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths);

    std::string generate(int year, const std::string& format_name);

private:
    YearlyDataReader m_reader;
    // 3. 使用模板类实例化年度插件加载器
    PluginLoader<IYearlyReportFormatter> m_plugin_manager;
};

#endif // YEARLY_REPORT_GENERATOR_H