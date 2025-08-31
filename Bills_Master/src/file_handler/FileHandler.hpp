// FileHandler.hpp

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @class FileHandler
 * @brief 一个用于处理文件和目录操作的工具类。
 */
class FileHandler {
public:
    FileHandler() = default;

    /**
     * @brief 根据给定的路径字符串和文件后缀，查找所有匹配的文件。
     *
     * 1. 如果路径指向一个单独的文件，它会检查该文件是否匹配指定的后缀名。
     * 2. 如果路径指向一个目录，它会递归地搜索该目录及其所有子目录，找出所有匹配后缀名的文件。
     *
     * @param input_path_str 用户提供的路径字符串。
     * @param extension 要查找的文件的后缀名 (例如, ".txt" 或 ".json")。
     * @return 返回一个包含所有找到的匹配文件路径 (fs::path) 的向量。
     * @throws std::runtime_error 如果给定的路径不存在，或者当路径指向单个文件但后缀不匹配时。
     */
    std::vector<fs::path> find_files_by_extension(const std::string& input_path_str, const std::string& extension);
};

#endif // FILE_HANDLER_H