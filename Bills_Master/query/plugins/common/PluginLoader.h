// query/plugins/common/PluginLoader.h
#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

template<typename T>
class PluginLoader {
public:
    // 构造函数现在接收一个插件后缀名，用于区分不同类型的插件
    explicit PluginLoader(const std::string& plugin_suffix) 
        : m_plugin_suffix(plugin_suffix) {}

    ~PluginLoader() {
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

    // 从指定目录加载所有插件
    void loadPluginsFromDirectory(const std::string& plugin_path) {
        if (!std::filesystem::exists(plugin_path) || !std::filesystem::is_directory(plugin_path)) {
            // 首次加载时目录不存在是正常情况，不打印警告
            return;
        }
        std::cout << "Scanning for plugins with suffix '" << m_plugin_suffix << "' in: " << plugin_path << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
            if (entry.is_regular_file()) {
                loadPlugin(entry.path().string());
            }
        }
    }

    // 从指定文件路径列表加载插件
    void loadPluginsFromFiles(const std::vector<std::string>& plugin_file_paths) {
        for (const auto& path : plugin_file_paths) {
            loadPlugin(path);
        }
    }

    // 加载单个插件
    bool loadPlugin(const std::string& plugin_file_path) {
        std::filesystem::path path(plugin_file_path);
        // 检查文件是否存在以及文件名是否包含预期的后缀
        if (!std::filesystem::is_regular_file(path) || path.stem().string().rfind(m_plugin_suffix) == std::string::npos) {
            return false;
        }

        LibraryHandle handle = nullptr;
#ifdef _WIN32
        if(path.extension() != ".dll") return false;
        handle = LoadLibraryA(plugin_file_path.c_str());
#else
        if(path.extension() != ".so") return false;
        handle = dlopen(plugin_file_path.c_str(), RTLD_LAZY);
#endif

        if (!handle) {
            // 错误处理逻辑保持不变
            std::cerr << "Error loading plugin: " << plugin_file_path << std::endl;
#ifdef _WIN32
            // ... Windows specific error message ...
#else
            std::cerr << "  -> System Error: " << dlerror() << std::endl;
#endif
            return false;
        }

        std::string format_name = getFormatNameFromFile(path);
        // 动态构建工厂函数名
        std::string function_name = "create_" + format_name + m_plugin_suffix;
        
        CreateFunc create_func = nullptr;
#ifdef _WIN32
        create_func = (CreateFunc)GetProcAddress(handle, function_name.c_str());
#else
        create_func = (CreateFunc)dlsym(handle, function_name.c_str());
#endif

        if (!create_func) {
            std::cerr << "Error: Could not find function '" << function_name << "' in " << plugin_file_path << std::endl;
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
            return false;
        }
        
        m_factories[format_name] = {handle, create_func};
        std::cout << "  -> Loaded plugin '" << format_name << "' with suffix '" << m_plugin_suffix << "' from " << path.filename().string() << std::endl;
        return true;
    }

    // 创建一个格式化器实例
    std::unique_ptr<T> createFormatter(const std::string& format_name) {
        auto it = m_factories.find(format_name);
        if (it != m_factories.end()) {
            return std::unique_ptr<T>(it->second.second());
        }
        return nullptr;
    }

    // 检查特定格式的插件是否可用
    bool isFormatAvailable(const std::string& format_name) const {
        return m_factories.count(format_name) > 0;
    }

private:
    // 使用模板参数 T 来定义创建函数的类型
    using CreateFunc = T* (*)(); 
#ifdef _WIN32
    using LibraryHandle = HMODULE;
#else
    using LibraryHandle = void*;
#endif

    // map 的 value 也使用模板参数 T
    std::map<std::string, std::pair<LibraryHandle, CreateFunc>> m_factories;
    std::string m_plugin_suffix; // 用于区分插件类型, e.g., "_month_formatter"

    // 从文件名中提取格式名称 (例如 "md_month_formatter.dll" -> "md")
    std::string getFormatNameFromFile(const std::filesystem::path& file_path) {
        std::string filename = file_path.stem().string();
        size_t suffix_pos = filename.rfind(m_plugin_suffix);
        if (suffix_pos != std::string::npos) {
            return filename.substr(0, suffix_pos);
        }
        return filename;
    }
};

#endif // PLUGIN_LOADER_H