#include "common/pch.h"
// YearlyReportGenerator.cpp
#include "YearlyReportGenerator.h"
#include <stdexcept>
#include <iostream>

// 构造函数1: 从目录扫描
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::string& plugin_directory_path)
    : m_reader(db_connection),
      // 1. 在初始化列表中传入年度插件的后缀
      m_plugin_manager("_year_formatter")
{
    std::cout << "YearlyReportGenerator initialized by scanning directory." << std::endl;
    // 2. 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromDirectory(plugin_directory_path);
}

// 构造函数2: 从文件列表加载
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths)
    : m_reader(db_connection),
      // 1. 在初始化列表中传入年度插件的后缀
      m_plugin_manager("_year_formatter")
{
    std::cout << "YearlyReportGenerator initialized with specific plugins." << std::endl;
    // 2. 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromFiles(plugin_file_paths);
}

std::string YearlyReportGenerator::generate(int year, const std::string& format_name) {
    YearlyReportData data = m_reader.read_yearly_data(year);

    auto formatter = m_plugin_manager.createFormatter(format_name);

    if (!formatter) {
        throw std::runtime_error("Failed to create formatter for format: " + format_name + ". Is the required plugin loaded?");
    }

    return formatter->format_report(data);
}