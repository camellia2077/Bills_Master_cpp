// query/components/yearly_report/YearlyReportGenerator.cpp
#include "YearlyReportGenerator.hpp"
#include <stdexcept>
#include <iostream>

// 构造函数1: 从目录扫描
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path)
    : m_reader(db_connection),
      // 在初始化列表中传入年度插件的后缀
      m_plugin_manager("_year_formatter")
{
    // 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromDirectory(plugin_directory_path);
}

// 构造函数2: 从文件列表加载
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths)
    : m_reader(db_connection),
      // 在初始化列表中传入年度插件的后缀
      m_plugin_manager("_year_formatter")
{
    // 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromFiles(plugin_file_paths);
}

std::string YearlyReportGenerator::generate(int year, const std::string& format_name) {
    YearlyReportData data = m_reader.read_yearly_data(year);

    auto formatter = m_plugin_manager.createFormatter(format_name);

    if (!formatter) {
        std::string expected_dll_name = format_name + "_year_formatter";
        #ifdef _WIN32
            expected_dll_name += ".dll";
        #else
            expected_dll_name += ".so";
        #endif
        throw std::runtime_error("Yearly formatter for '" + format_name + "' is not available. Please ensure that the plugin file '" + expected_dll_name + "' exists in the plugins directory.");
    }

    return formatter->format_report(data);
}