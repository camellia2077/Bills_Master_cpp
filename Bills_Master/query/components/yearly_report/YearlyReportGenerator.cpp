// YearlyReportGenerator.cpp
#include "YearlyReportGenerator.h"
#include <stdexcept>
#include <iostream>

// --- Constructor 1  ---
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path)
    : m_reader(db_connection),
      m_plugin_manager(plugin_directory_path)
{
    std::cout << "YearlyReportGenerator initialized by scanning directory." << std::endl;
}

// --- Constructor 2 ---
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths)
    : m_reader(db_connection),
      m_plugin_manager() // Now correctly calls the new default constructor
{
    std::cout << "YearlyReportGenerator initialized with specific plugins." << std::endl;
    for (const auto& path : plugin_file_paths) {
        m_plugin_manager.loadPlugin(path); // Now correctly calls the new public method
    }
}

// --- [FIXED] `generate` method implementation ---
// The signature now matches the header file: it takes a const std::string&
std::string YearlyReportGenerator::generate(int year, const std::string& format_name) {
    YearlyReportData data = m_reader.read_yearly_data(year);

    auto formatter = m_plugin_manager.createFormatter(format_name);

    if (!formatter) {
        throw std::runtime_error("Failed to create formatter for format: " + format_name + ". Is the required plugin loaded?");
    }

    return formatter->format_report(data);
}