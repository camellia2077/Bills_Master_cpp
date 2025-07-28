// query/month/month_format/FormatterPluginManager.cpp
#include "FormatterPluginManager.h"
#include <iostream>
#include <stdexcept>

// --- 构造函数：加载插件 ---
FormatterPluginManager::FormatterPluginManager(const std::string& plugin_path, const std::string& plugin_type) {
    // 根据传入的类型，构建期望的文件名后缀
    // 例如，如果 plugin_type 是 "month"，m_plugin_suffix 将是 "_month_formatter"
    m_plugin_suffix = "_" + plugin_type + "_formatter";
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
    std::cout << "Required plugin suffix: " << m_plugin_suffix << std::endl;

#ifdef _WIN32
    const std::string extension = ".dll";
#else
    const std::string extension = ".so";
#endif

    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        std::string filename_stem = entry.path().stem().string(); // 获取文件名主体，例如 "md_month_formatter"
        
        // ==========================================================
        // ===== MODIFICATION START: Filter plugins by suffix   ====
        // ==========================================================
        // 检查文件是否为常规文件，扩展名是否正确，并且文件名是否包含正确的类型后缀
        if (entry.is_regular_file() && 
            entry.path().extension() == extension &&
            filename_stem.rfind(m_plugin_suffix) != std::string::npos) {
        // ==========================================================
        // ===== MODIFICATION END ===================================
        // ==========================================================
            LibraryHandle handle = nullptr;
#ifdef _WIN32
            handle = LoadLibraryA(entry.path().string().c_str());
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
        return std::unique_ptr<IMonthReportFormatter>(it->second.second());
    }
    return nullptr;
}


// --- 辅助函数：从dll提取格式名 ---
// 这个函数无需修改。它仍然正确地从 "md_month_formatter" 中提取 "md"。
std::string FormatterPluginManager::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string(); 
    size_t first_underscore_pos = filename.find('_');
    if (first_underscore_pos != std::string::npos) {
        return filename.substr(0, first_underscore_pos);
    }
    return filename;
}