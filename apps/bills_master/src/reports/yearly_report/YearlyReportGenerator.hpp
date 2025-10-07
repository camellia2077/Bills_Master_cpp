// reports/components/yearly_report/YearlyReportGenerator.hpp
#ifndef YEARLY_REPORT_GENERATOR_HPP
#define YEARLY_REPORT_GENERATOR_HPP

#include <string>
#include <vector>
#include <sqlite3.h>
#include "YearQuery.hpp"
#include "reports/plugins/common/PluginLoader.hpp" // 1. 包含新的通用加载器
#include "reports/plugins/year_formatters/IYearlyReportFormatter.hpp" 

class YearlyReportGenerator {
public:
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path);
    explicit YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths);

    std::string generate(int year, const std::string& format_name);

private:
    YearQuery m_reader;
    // 3. 使用模板类实例化年度插件加载器
    PluginLoader<IYearlyReportFormatter> m_plugin_manager;
};

#endif // YEARLY_REPORT_GENERATOR_HPP