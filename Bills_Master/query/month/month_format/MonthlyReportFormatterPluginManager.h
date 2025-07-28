// query/month/month_format/MonthlyReportFormatterPluginManager.h
#ifndef MONTHLY_REPORT_FORMATTER_PLUGIN_MANAGER_H
#define MONTHLY_REPORT_FORMATTER_PLUGIN_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <filesystem>
#include "IMonthReportFormatter.h" // [cite: 1]

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// CHANGED: Class renamed to be specific to monthly reports.
class MonthlyReportFormatterPluginManager {
public:
    // CHANGED: Add a default constructor for list-based loading.
    explicit MonthlyReportFormatterPluginManager();

    // CHANGED: Constructor now only needs the directory path.
    explicit MonthlyReportFormatterPluginManager(const std::string& plugin_directory_path);

    ~MonthlyReportFormatterPluginManager();
    
    // CHANGED: Add a method to load a single plugin by its full path.
    bool loadPlugin(const std::string& plugin_file_path);
    
    std::unique_ptr<IMonthReportFormatter> createFormatter(const std::string& format_name);

private:
    using FormatterCreateFunc = IMonthReportFormatter* (*)();
    using LibraryHandle = void*; // Simplified for cross-platform, assuming HMODULE can be cast

    std::map<std::string, std::pair<LibraryHandle, FormatterCreateFunc>> m_factories;
    std::string m_plugin_suffix; 

    void loadPluginsFromDirectory(const std::string& plugin_path);
    std::string getFormatNameFromFile(const std::filesystem::path& file_path);
};

#endif // MONTHLY_REPORT_FORMATTER_PLUGIN_MANAGER_H