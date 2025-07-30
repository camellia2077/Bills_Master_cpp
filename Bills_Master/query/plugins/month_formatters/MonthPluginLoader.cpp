// query/month/month_format/MonthPluginLoader.cpp
#include "MonthPluginLoader.h"
#include <iostream>
#include <stdexcept>

MonthPluginLoader::MonthPluginLoader(const std::string& plugin_directory_path) : m_plugin_suffix("_month_formatter") {
    loadPluginsFromDirectory(plugin_directory_path);
}
MonthPluginLoader::MonthPluginLoader(const std::vector<std::string>& plugin_file_paths) : m_plugin_suffix("_month_formatter") {
    for (const auto& path : plugin_file_paths) {
        loadPlugin(path);
    }
}


bool MonthPluginLoader::isFormatAvailable(const std::string& format_name) const {
    return m_factories.count(format_name) > 0;
}

// --- 修改开始: 移除析构函数中的类型转换 ---
MonthPluginLoader::~MonthPluginLoader() {
    for (auto const& [key, val] : m_factories) {
        if (val.first) {
#ifdef _WIN32
            FreeLibrary(val.first); // 不再需要 static_cast
#else
            dlclose(val.first);
#endif
        }
    }
}
// --- 修改结束 ---

bool MonthPluginLoader::loadPlugin(const std::string& plugin_file_path) {
    std::filesystem::path path(plugin_file_path);
    if (!std::filesystem::is_regular_file(path) || path.stem().string().rfind(m_plugin_suffix) == std::string::npos) {
        return false;
    }

#ifdef _WIN32
    if(path.extension() != ".dll") return false;
    LibraryHandle handle = LoadLibraryA(plugin_file_path.c_str());
#else
    if(path.extension() != ".so") return false;
    LibraryHandle handle = dlopen(plugin_file_path.c_str(), RTLD_LAZY);
#endif

    if (!handle) {
        std::cerr << "Error loading monthly plugin: " << plugin_file_path << std::endl;
#ifdef _WIN32
        DWORD error_code = GetLastError();
        LPSTR error_message = nullptr;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&error_message, 0, nullptr);
        if (error_message) {
            std::cerr << "  -> System Error: " << error_message << std::endl;
            LocalFree(error_message);
        }
#else
        std::cerr << "  -> System Error: " << dlerror() << std::endl;
#endif
        return false;
    }

    std::string format_name = getFormatNameFromFile(path);
    std::string function_name = "create_" + format_name + "_month_formatter";
    
    FormatterCreateFunc create_func = nullptr;
#ifdef _WIN32
    // --- 修改开始: 移除 GetProcAddress 的类型转换 ---
    create_func = (FormatterCreateFunc)GetProcAddress(handle, function_name.c_str());
    // --- 修改结束 ---
#else
    create_func = (FormatterCreateFunc)dlsym(handle, function_name.c_str());
#endif

    if (!create_func) {
        std::cerr << "Error: Could not find the expected function '" << function_name 
                  << "' in " << plugin_file_path << "." << std::endl;
        std::cerr << "  -> Please ensure the plugin exports a function with the signature: "
                  << "extern \"C\" IMonthReportFormatter* " << function_name << "()" << std::endl;
#ifdef _WIN32
        // --- 修改开始: 移除 FreeLibrary 的类型转换 ---
        FreeLibrary(handle);
        // --- 修改结束 ---
#else
        dlclose(handle);
#endif
        return false;
    }
    
    m_factories[format_name] = {handle, create_func};
    std::cout << "  -> Loaded MONTHLY plugin '" << format_name << "' from " << path.filename().string() << std::endl;
    return true;
}

void MonthPluginLoader::loadPluginsFromDirectory(const std::string& plugin_path) {
    if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
        std::cerr << "Warning: Plugin directory '" << plugin_path << "' does not exist. No monthly plugins loaded." << std::endl;
        return;
    }

    std::cout << "Scanning for MONTHLY plugins in: " << plugin_path << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        loadPlugin(entry.path().string());
    }
}

std::unique_ptr<IMonthReportFormatter> MonthPluginLoader::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        return std::unique_ptr<IMonthReportFormatter>(it->second.second());
    }
    return nullptr;
}

std::string MonthPluginLoader::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string(); 
    size_t first_underscore_pos = filename.find('_');
    if (first_underscore_pos != std::string::npos) {
        return filename.substr(0, first_underscore_pos);
    }
    return filename;
}