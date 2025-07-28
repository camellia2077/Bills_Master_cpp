// query/year/year_format/YearlyReportFormatterPluginManager.h
#ifndef YEARLY_REPORT_FORMATTER_PLUGIN_MANAGER_H
#define YEARLY_REPORT_FORMATTER_PLUGIN_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <filesystem>
#include "IYearlyReportFormatter.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class YearlyReportFormatterPluginManager {
public:
    /**
     * @brief [FIXED] Default constructor to create an empty manager.
     */
    explicit YearlyReportFormatterPluginManager();

    /**
     * @brief Constructor to scan a directory and load all yearly plugins.
     * @param plugin_directory_path Path to the directory containing plugin libraries.
     */
    explicit YearlyReportFormatterPluginManager(const std::string& plugin_directory_path);

    ~YearlyReportFormatterPluginManager();
    
    /**
     * @brief [FIXED] Loads a single specified plugin file.
     * @param plugin_file_path The full path to a single plugin file (.dll or .so).
     * @return True if loading was successful, false otherwise.
     */
    bool loadPlugin(const std::string& plugin_file_path);
    
    /**
     * @brief Creates a formatter instance by its format name.
     * @param format_name The name of the format (e.g., "md", "tex").
     * @return A unique_ptr to the formatter interface.
     */
    std::unique_ptr<IYearlyReportFormatter> createFormatter(const std::string& format_name);

private:
    using FormatterCreateFunc = IYearlyReportFormatter* (*)();
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

#endif // YEARLY_REPORT_FORMATTER_PLUGIN_MANAGER_H