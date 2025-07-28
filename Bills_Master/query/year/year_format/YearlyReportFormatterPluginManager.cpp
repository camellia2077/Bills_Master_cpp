// query/year/year_format/YearlyReportFormatterPluginManager.cpp
#include "YearlyReportFormatterPluginManager.h"
#include <iostream>
#include <stdexcept>

// --- [FIXED] Default constructor implementation ---
YearlyReportFormatterPluginManager::YearlyReportFormatterPluginManager() {
    m_plugin_suffix = "_year_formatter";
}

// --- Directory-scanning constructor implementation ---
YearlyReportFormatterPluginManager::YearlyReportFormatterPluginManager(const std::string& plugin_directory_path) {
    m_plugin_suffix = "_year_formatter";
    loadPluginsFromDirectory(plugin_directory_path);
}

// --- Destructor (no change) ---
YearlyReportFormatterPluginManager::~YearlyReportFormatterPluginManager() {
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

// --- [FIXED] `loadPlugin` implementation ---
bool YearlyReportFormatterPluginManager::loadPlugin(const std::string& plugin_file_path) {
    std::filesystem::path path(plugin_file_path);
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        return false;
    }
    std::string filename_stem = path.stem().string();
    if (filename_stem.rfind(m_plugin_suffix) == std::string::npos) {
        return false;
    }
    
    LibraryHandle handle = nullptr;
#ifdef _WIN32
    handle = LoadLibraryA(plugin_file_path.c_str());
#else
    handle = dlopen(plugin_file_path.c_str(), RTLD_LAZY);
#endif
    if (!handle) {
        std::cerr << "Error loading plugin: " << plugin_file_path << std::endl;
        return false;
    }

    std::string format_name = getFormatNameFromFile(path);
    std::string function_name = "create_" + format_name + "_year_formatter";
    
    FormatterCreateFunc create_func = nullptr;
#ifdef _WIN32
    create_func = (FormatterCreateFunc)GetProcAddress(handle, function_name.c_str());
#else
    create_func = (FormatterCreateFunc)dlsym(handle, function_name.c_str());
#endif

    if (!create_func) {
        std::cerr << "Error: Could not find '" << function_name << "' function in " << plugin_file_path << std::endl;
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }
    
    m_factories[format_name] = {handle, create_func};
    std::cout << "  -> Loaded YEARLY plugin '" << format_name << "' from " << path.filename().string() << std::endl;
    return true;
}

// --- Renamed from `loadPlugins` to `loadPluginsFromDirectory` ---
void YearlyReportFormatterPluginManager::loadPluginsFromDirectory(const std::string& plugin_path) {
    if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        if (entry.is_regular_file()) {
            loadPlugin(entry.path().string());
        }
    }
}

// --- `createFormatter` and `getFormatNameFromFile` (no change) ---
std::unique_ptr<IYearlyReportFormatter> YearlyReportFormatterPluginManager::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        return std::unique_ptr<IYearlyReportFormatter>(it->second.second());
    }
    return nullptr;
}

std::string YearlyReportFormatterPluginManager::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string(); 
    size_t first_underscore_pos = filename.find('_');
    if (first_underscore_pos != std::string::npos) {
        return filename.substr(0, first_underscore_pos);
    }
    return filename;
}