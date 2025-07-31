// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "MonthQuery.h"
#include "query/plugins/common/PluginLoader.h" // 1. Include the new generic loader
#include "query/plugins/month_formatters/IMonthReportFormatter.h" // 2. Include the interface

class MonthlyReportGenerator {
public:
    explicit MonthlyReportGenerator(sqlite3* db_connection, const std::string& plugin_path);
    explicit MonthlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths);

    std::string generate(int year, int month, const std::string& format_name);

private:
    MonthQuery m_reader;
    // 3. Use the templated PluginLoader for the monthly formatter interface
    PluginLoader<IMonthReportFormatter> m_plugin_manager;
};

#endif // MONTHLY_REPORT_GENERATOR_H