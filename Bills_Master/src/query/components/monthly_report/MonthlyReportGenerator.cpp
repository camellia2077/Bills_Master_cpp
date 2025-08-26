
// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.hpp"
#include <stdexcept>

// 构造函数1: 从目录扫描
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection, const std::string& plugin_path)
    : m_reader(db_connection), 
      // 1. 在初始化列表中传入月度插件的后缀
      m_plugin_manager("_month_formatter") 
{
    // 2. 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromDirectory(plugin_path);
}

// 构造函数2: 从文件列表加载
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection, const std::vector<std::string>& plugin_file_paths)
    : m_reader(db_connection), 
      // 1. 同样，在初始化列表中传入月度插件的后缀
      m_plugin_manager("_month_formatter") 
{
    // 2. 在函数体中调用加载方法
    m_plugin_manager.loadPluginsFromFiles(plugin_file_paths);
}

std::string MonthlyReportGenerator::generate(int year, int month, const std::string& format_name) {
    MonthlyReportData data = m_reader.read_monthly_data(year, month);
    auto formatter = m_plugin_manager.createFormatter(format_name);

    if (!formatter) {
        throw std::runtime_error("Unsupported or unloaded report format specified: " + format_name);
    }

    return formatter->format_report(data);
}