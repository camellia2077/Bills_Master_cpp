// query/month/month_format/ReportFormatterFactory.cpp
#include "ReportFormatterFactory.h"
#include <iostream>
#include <stdexcept>

// --- 构造函数：加载插件 ---
ReportFormatterFactory::ReportFormatterFactory(const std::string& plugin_path) {
    loadPlugins(plugin_path);
}

// --- 析构函数：卸载插件 ---
ReportFormatterFactory::~ReportFormatterFactory() {
    for (auto const& [key, val] : m_factories) {
        if (val.first) {
#ifdef _WIN32
            FreeLibrary(val.first);
#else
            dlclose(val.first);
#endif
        }
    }
}

// --- 核心方法：加载所有插件 ---
void ReportFormatterFactory::loadPlugins(const std::string& plugin_path) {
    if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
        std::cerr << "Warning: Plugin directory '" << plugin_path << "' does not exist. No plugins loaded." << std::endl;
        return;
    }

    std::cout << "Scanning for plugins in: " << plugin_path << std::endl;

#ifdef _WIN32
    const std::string extension = ".dll";
#else
    const std::string extension = ".so";
#endif

    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            LibraryHandle handle = nullptr;
#ifdef _WIN32
            // ==========================================================
            // ===== MODIFICATION START: Use LoadLibraryA explicitly ====
            // ==========================================================
            handle = LoadLibraryA(entry.path().string().c_str());
            // ==========================================================
            // ===== MODIFICATION END ===================================
            // ==========================================================
#else
            handle = dlopen(entry.path().string().c_str(), RTLD_LAZY);
#endif
            if (!handle) {
                std::cerr << "Error loading plugin: " << entry.path().string() << std::endl;
                continue;
            }

            FormatterCreateFunc create_func = nullptr;
#ifdef _WIN32
            create_func = (FormatterCreateFunc)GetProcAddress(handle, "create_formatter");
#else
            create_func = (FormatterCreateFunc)dlsym(handle, "create_formatter");
#endif

            if (!create_func) {
                std::cerr << "Error: Could not find 'create_formatter' function in " << entry.path().string() << std::endl;
#ifdef _WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                continue;
            }

            std::string format_name = getFormatNameFromFile(entry.path());
            m_factories[format_name] = {handle, create_func};
            std::cout << "  -> Loaded plugin '" << format_name << "' from " << entry.path().filename().string() << std::endl;
        }
    }
}

// --- 核心方法：创建格式化器实例 ---
std::unique_ptr<IMonthReportFormatter> ReportFormatterFactory::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        // 如果找到了，调用存储的函数指针来创建对象
        return std::unique_ptr<IMonthReportFormatter>(it->second.second());
    }
    // 未找到支持的格式
    return nullptr;
}


// --- 辅助函数：从文件名提取格式名 ---
std::string ReportFormatterFactory::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string(); // 获取文件名（不含扩展名）
    // 我们的插件命名可能是 "md_formatter.dll"，需要去掉 "_formatter"
    const std::string suffix = "_formatter";
    if (filename.size() > suffix.size() && filename.substr(filename.size() - suffix.size()) == suffix) {
        return filename.substr(0, filename.size() - suffix.size());
    }
    return filename; // 如果命名不规范，直接返回文件名作为格式名
}