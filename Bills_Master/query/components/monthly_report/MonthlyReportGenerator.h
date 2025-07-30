// MonthlyReportGenerator.h
#ifndef MONTHLY_REPORT_GENERATOR_H
#define MONTHLY_REPORT_GENERATOR_H

#include <string>
#include <vector> // CHANGED: Add include for vector
#include <sqlite3.h>
// CHANGED: Include the new specific plugin manager header
#include "query/plugins/month_formatters/MonthPluginLoader.h" 
#include "MonthQuery.h"

class MonthlyReportGenerator {
public:
    // Constructor for directory scanning
    explicit MonthlyReportGenerator(sqlite3* db_connection, const std::string& plugin_path);

    // CHANGED: Add constructor to accept a list of plugin files
    explicit MonthlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths);

    std::string generate(int year, int month, const std::string& format_name);

private:
    MonthQuery m_reader;
    // CHANGED: Use the new specific plugin manager class
    MonthPluginLoader m_plugin_manager;
};

#endif // MONTHLY_REPORT_GENERATOR_H