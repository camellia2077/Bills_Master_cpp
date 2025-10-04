// reports/components/monthly_report/MonthlyReportGenerator.hpp
#ifndef MONTHLY_REPORT_GENERATOR_HPP
#define MONTHLY_REPORT_GENERATOR_HPP

#include <string>
#include <vector>
#include <sqlite3.h>
#include "MonthQuery.hpp"
#include "reports/plugins/common/PluginLoader.hpp" 
#include "reports/plugins/month_formatters/IMonthReportFormatter.hpp"

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

#endif // MONTHLY_REPORT_GENERATOR_HPP