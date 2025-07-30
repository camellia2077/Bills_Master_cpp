// query/month/month_format/MonthPluginLoader.h
#ifndef MONTH_PLUGIN_LOADER_H
#define MONTH_PLUGIN_LOADER_H

#include <memory>
#include <string>
#include <map>
#include <filesystem>
#include "IMonthReportFormatter.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class MonthPluginLoader {
public:
    explicit MonthPluginLoader() = default; //
    explicit MonthPluginLoader(const std::string& plugin_directory_path);
    explicit MonthPluginLoader(const std::vector<std::string>& plugin_file_paths);

    ~MonthPluginLoader();
    
    bool loadPlugin(const std::string& plugin_file_path);
    std::unique_ptr<IMonthReportFormatter> createFormatter(const std::string& format_name);

    // --- 新增接口: 检查特定格式的插件是否可用 ---
    bool isFormatAvailable(const std::string& format_name) const;

private:
    using FormatterCreateFunc = IMonthReportFormatter* (*)();
#ifdef _WIN32
    using LibraryHandle = HMODULE;
#else
    using LibraryHandle = void*;
#endif

    std::map<std::string, std::pair<LibraryHandle, FormatterCreateFunc>> m_factories;
    std::string m_plugin_suffix; 

    void loadPluginsFromDirectory(const std::string& plugin_path);
    std::string getFormatNameFromFile(const std::filesystem::path& file_path);
};

#endif // MONTHLY_REPORT_FORMATTER_PLUGIN_MANAGER_H