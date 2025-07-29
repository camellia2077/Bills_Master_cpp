// query/year/year_format/YearPluginLoader.h
#ifndef YEAR_PLUGIN_Loader_H
#define YEAR_PLUGIN_Loader_H

#include <memory>
#include <string>
#include <map>
#include <filesystem>
#include <vector> // <--- 新增：包含 vector 头文件以解决编译错误
#include "IYearlyReportFormatter.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class YearPluginLoader {
public:
    explicit YearPluginLoader() = default;
    explicit YearPluginLoader(const std::string& plugin_directory_path);
    explicit YearPluginLoader(const std::vector<std::string>& plugin_file_paths);
    ~YearPluginLoader();
    
    bool loadPlugin(const std::string& plugin_file_path);
    std::unique_ptr<IYearlyReportFormatter> createFormatter(const std::string& format_name);

    bool isFormatAvailable(const std::string& format_name) const;

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
