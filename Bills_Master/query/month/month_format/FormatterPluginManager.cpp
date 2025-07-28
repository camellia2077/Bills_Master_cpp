// query/month/month_format/FormatterPluginManager.cpp
#include "FormatterPluginManager.h"
#include <iostream>
#include <stdexcept>

// --- 构造函数：加载插件 ---
FormatterPluginManager::FormatterPluginManager(const std::string& plugin_path) {
    loadPlugins(plugin_path);
}

// --- 析构函数：卸载插件 ---
FormatterPluginManager::~FormatterPluginManager() {
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
void FormatterPluginManager::loadPlugins(const std::string& plugin_path) {
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
std::unique_ptr<IMonthReportFormatter> FormatterPluginManager::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        // 如果找到了，调用存储的函数指针来创建对象
        return std::unique_ptr<IMonthReportFormatter>(it->second.second());
    }
    // 未找到支持的格式
    return nullptr;
}


// --- 辅助函数：从dll提取格式名 ---
std::string FormatterPluginManager::getFormatNameFromFile(const std::filesystem::path& file_path) {
    // 获取文件名，不含扩展名。例如 "md_month_formatter"
    std::string filename = file_path.stem().string(); 

    // 查找第一个下划线的位置
    size_t first_underscore_pos = filename.find('_');

    // 如果找到了下划线
    if (first_underscore_pos != std::string::npos) {
        // 返回从字符串开始到第一个下划线之前的部分
        // 例如，对于 "md_month_formatter", 将返回 "md"
        return filename.substr(0, first_underscore_pos);
    }

    // 如果文件名中没有下划线（为了兼容旧的命名或意外情况），
    // 则直接返回完整的文件名作为格式名，作为一种安全的回退机制。
    return filename;
}