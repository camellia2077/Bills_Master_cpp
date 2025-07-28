// query/month/month_format/MonthlyReportFormatterPluginManager.cpp
#include "MonthlyReportFormatterPluginManager.h"
#include <iostream>
#include <stdexcept>

// [MODIFIED] Default constructor implementation
MonthlyReportFormatterPluginManager::MonthlyReportFormatterPluginManager() {
    m_plugin_suffix = "_month_formatter";
}

// [MODIFIED] Directory-scanning constructor now hard-codes the suffix
MonthlyReportFormatterPluginManager::MonthlyReportFormatterPluginManager(const std::string& plugin_directory_path) {
    m_plugin_suffix = "_month_formatter";
    loadPluginsFromDirectory(plugin_directory_path);
}

// Destructor implementation
MonthlyReportFormatterPluginManager::~MonthlyReportFormatterPluginManager() {
    for (auto const& [key, val] : m_factories) {
        if (val.first) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(val.first));
#else
            dlclose(val.first);
#endif
        }
    }
}

// [NEW] Implementation for loading a single plugin file
bool MonthlyReportFormatterPluginManager::loadPlugin(const std::string& plugin_file_path) {
    std::filesystem::path path(plugin_file_path);

    // Check if the file is a regular file with the correct suffix
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
        return false;
    }

    // [MODIFIED] Dynamically create the expected function name
    std::string format_name = getFormatNameFromFile(path);
    std::string function_name = "create_" + format_name + "_month_formatter";
    
    FormatterCreateFunc create_func = nullptr;
#ifdef _WIN32
    create_func = (FormatterCreateFunc)GetProcAddress(static_cast<HMODULE>(handle), function_name.c_str());
#else
    create_func = (FormatterCreateFunc)dlsym(handle, function_name.c_str());
#endif

    if (!create_func) {
        std::cerr << "Error: Could not find '" << function_name << "' function in " << plugin_file_path << std::endl;
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return false;
    }
    
    m_factories[format_name] = {handle, create_func};
    std::cout << "  -> Loaded MONTHLY plugin '" << format_name << "' from " << path.filename().string() << std::endl;
    return true;
}

// [MODIFIED] Renamed and refactored to use the new `loadPlugin` method
void MonthlyReportFormatterPluginManager::loadPluginsFromDirectory(const std::string& plugin_path) {
    if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
        std::cerr << "Warning: Plugin directory '" << plugin_path << "' does not exist. No monthly plugins loaded." << std::endl;
        return;
    }

    std::cout << "Scanning for MONTHLY plugins in: " << plugin_path << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        loadPlugin(entry.path().string());
    }
}

// createFormatter implementation remains conceptually the same
std::unique_ptr<IMonthReportFormatter> MonthlyReportFormatterPluginManager::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        return std::unique_ptr<IMonthReportFormatter>(it->second.second());
    }
    return nullptr;
}

// getFormatNameFromFile implementation remains the same
std::string MonthlyReportFormatterPluginManager::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string(); 
    size_t first_underscore_pos = filename.find('_');
    if (first_underscore_pos != std::string::npos) {
        return filename.substr(0, first_underscore_pos);
    }
    return filename;
}