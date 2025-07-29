// query/year/year_format/YearPluginLoader.cpp
#include "YearPluginLoader.h"
#include <iostream>
#include <stdexcept>



// --- Constructor for directory scanning ---
YearPluginLoader::YearPluginLoader(const std::string& plugin_directory_path) : m_plugin_suffix("_year_formatter") {
    loadPluginsFromDirectory(plugin_directory_path);
}

// --- Constructor for file list loading ---
YearPluginLoader::YearPluginLoader(const std::vector<std::string>& plugin_file_paths) : m_plugin_suffix("_year_formatter") {
    for (const auto& path : plugin_file_paths) {
        loadPlugin(path);
    }
}

// --- Destructor ---
YearPluginLoader::~YearPluginLoader() {
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

// --- loadPlugin implementation ---
bool YearPluginLoader::loadPlugin(const std::string& plugin_file_path) {
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
        std::cerr << "Error loading yearly plugin: " << plugin_file_path << std::endl;
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
    std::string function_name = "create_" + format_name + "_year_formatter";

    FormatterCreateFunc create_func = nullptr;
#ifdef _WIN32
    create_func = (FormatterCreateFunc)GetProcAddress(handle, function_name.c_str());
#else
    create_func = (FormatterCreateFunc)dlsym(handle, function_name.c_str());
#endif

    if (!create_func) {
        std::cerr << "Error: Could not find the expected function '" << function_name
                  << "' in " << plugin_file_path << "." << std::endl;
        std::cerr << "  -> Please ensure the plugin exports a function with the signature: "
                  << "extern \"C\" IYearlyReportFormatter* " << function_name << "()" << std::endl;
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

// --- loadPluginsFromDirectory implementation ---
void YearPluginLoader::loadPluginsFromDirectory(const std::string& plugin_path) {
    if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
        if (entry.is_regular_file()) {
            loadPlugin(entry.path().string());
        }
    }
}

// --- createFormatter implementation ---
std::unique_ptr<IYearlyReportFormatter> YearPluginLoader::createFormatter(const std::string& format_name) {
    auto it = m_factories.find(format_name);
    if (it != m_factories.end()) {
        return std::unique_ptr<IYearlyReportFormatter>(it->second.second());
    }
    return nullptr;
}

// --- getFormatNameFromFile implementation ---
std::string YearPluginLoader::getFormatNameFromFile(const std::filesystem::path& file_path) {
    std::string filename = file_path.stem().string();
    size_t first_underscore_pos = filename.find('_');
    if (first_underscore_pos != std::string::npos) {
        return filename.substr(0, first_underscore_pos);
    }
    return filename;
}

// --- isFormatAvailable implementation ---
bool YearPluginLoader::isFormatAvailable(const std::string& format_name) const {
    return m_factories.count(format_name) > 0;
}
