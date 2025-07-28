// query/month/month_format/FormatterPluginManager.h
#ifndef REPORT_FORMATTER_FACTORY_H
#define REPORT_FORMATTER_FACTORY_H

#include <memory>
#include <string>
#include <map>
#include <filesystem>
#include "IMonthReportFormatter.h"
#include "query/ReportFormat.h" 

// 根据操作系统包含不同的头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class FormatterPluginManager {
public:
    /**
     * @brief 构造函数，扫描插件目录并加载所有可用的格式化器插件。
     * @param plugin_path 指向包含插件动态库的目录的路径。
     */
    explicit FormatterPluginManager(const std::string& plugin_path = "plugins");

    /**
     * @brief 析构函数，负责卸载所有已加载的插件库。
     */
    ~FormatterPluginManager();

    /**
     * @brief 根据格式名称字符串创建一个格式化器实例。
     * @param format_name 格式的名称（例如 "md", "tex"）。
     * @return 如果找到插件，则返回一个指向格式化器实例的 unique_ptr；否则返回 nullptr。
     */
    std::unique_ptr<IMonthReportFormatter> createFormatter(const std::string& format_name);

private:
    // 定义一个函数指针类型，它必须与插件中导出的 create_formatter 函数签名完全一致。
    using FormatterCreateFunc = IMonthReportFormatter* (*)();

    // 定义一个平台无关的库句柄类型
#ifdef _WIN32
    using LibraryHandle = HMODULE;
#else
    using LibraryHandle = void*;
#endif

    // 用于存储加载的插件信息
    std::map<std::string, std::pair<LibraryHandle, FormatterCreateFunc>> m_factories;

    // 私有辅助函数
    void loadPlugins(const std::string& plugin_path);
    std::string getFormatNameFromFile(const std::filesystem::path& file_path);
};

#endif // REPORT_FORMATTER_FACTORY_H