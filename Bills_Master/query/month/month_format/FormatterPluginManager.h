// query/month/month_format/FormatterPluginManager.h

#ifndef REPORT_FORMATTER_FACTORY_H
#define REPORT_FORMATTER_FACTORY_H

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

class FormatterPluginManager {
public:
    /**
     * @brief 构造函数，扫描并加载特定类型的插件.
     * @param plugin_path 指向包含插件动态库的目录的路径。
     * @param plugin_type 插件的类型 (例如 "month" 或 "year")，用于匹配文件名。
     */
    explicit FormatterPluginManager(const std::string& plugin_path, const std::string& plugin_type);

    ~FormatterPluginManager();
    
    std::unique_ptr<IMonthReportFormatter> createFormatter(const std::string& format_name);

private:
    using FormatterCreateFunc = IMonthReportFormatter* (*)();
#ifdef _WIN32
    using LibraryHandle = HMODULE;
#else
    using LibraryHandle = void*;
#endif

    std::map<std::string, std::pair<LibraryHandle, FormatterCreateFunc>> m_factories;
    
    // 新增成员变量，用于存储期望的插件文件名后缀
    std::string m_plugin_suffix; 

    // 私有辅助函数
    void loadPlugins(const std::string& plugin_path);
    std::string getFormatNameFromFile(const std::filesystem::path& file_path);
};

#endif // REPORT_FORMATTER_FACTORY_H