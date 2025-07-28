// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.h"
#include <stdexcept>

// [FIXED] Constructor implementation for directory scanning.
// It now correctly initializes m_plugin_manager instead of the non-existent m_factory.
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection, const std::string& plugin_path)
    : m_reader(db_connection), 
      m_plugin_manager(plugin_path) // Correctly initializes the plugin manager with the directory path
{
}

// [FIXED] New constructor implementation for loading from a list of specific plugin files.
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths)
    : m_reader(db_connection), 
      m_plugin_manager() // Calls the default constructor of the plugin manager
{
    // Loop through the provided paths and load each plugin individually
    for (const auto& path : plugin_file_paths) {
        m_plugin_manager.loadPlugin(path);
    }
}

// generate method implementation
std::string MonthlyReportGenerator::generate(int year, int month, const std::string& format_name) {
    // Step 1: Use the internal reader to get data from the database (no change)
    MonthlyReportData data = m_reader.read_monthly_data(year, month);

    // [FIXED] Step 2: Use the new m_plugin_manager to create the formatter instance.
    auto formatter = m_plugin_manager.createFormatter(format_name);

    // Step 3: If the manager returns a nullptr, the plugin was not found (no change)
    if (!formatter) {
        throw std::runtime_error("Unsupported or unloaded report format specified: " + format_name);
    }

    // Step 4: Use the created formatter to generate the report (no change)
    return formatter->format_report(data);
}