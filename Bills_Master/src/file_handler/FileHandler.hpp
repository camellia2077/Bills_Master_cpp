// FileHandler.h

// 为了防止头文件被重复包含，使用标准的头文件保护宏
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>       // 引入 std::string 类，用于处理字符串
#include <vector>       // 引入 std::vector 容器，用于存储文件路径列表
#include <filesystem>   // 引入 C++17 的文件系统库，用于路径操作和文件查询

// 为 std::filesystem 命名空间创建一个更短的别名 "fs"，方便后续使用
namespace fs = std::filesystem;

/**
 * @class FileHandler
 * @brief 一个用于处理文件和目录操作的工具类。
 *
 * 这个类的主要功能是根据给定的路径查找文本文件。
 */
class FileHandler {
public:
    /**
     * @brief 默认构造函数。
     * * 使用 '= default' 关键字告诉编译器自动生成一个默认的构造函数。
     * 因为这个类没有任何成员变量需要特殊初始化，所以默认构造函数就足够了。
     */
    FileHandler() = default;

    /**
     * @brief 根据给定的路径字符串，查找所有 .txt 文件。
     *
     * 这个函数功能强大，可以处理两种情况：
     * 1. 如果路径指向一个单独的文件，它会检查该文件是否为 .txt 文件。
     * 2. 如果路径指向一个目录，它会递归地搜索该目录及其所有子目录，找出其中所有的 .txt 文件。
     *
     * @param input_path_str 用户提供的路径字符串（例如 "C:/Users/Test/file.txt" 或 "C:/Users/Test/Documents"）。
     * @return 返回一个包含所有找到的 .txt 文件路径 (fs::path) 的向量。如果未找到文件，则返回一个空向量。
     * @throws std::runtime_error 如果给定的路径不存在，或者当路径指向单个文件但该文件不是 .txt 文件时，会抛出此异常。
     */
    std::vector<fs::path> find_txt_files(const std::string& input_path_str);
};

#endif // FILE_HANDLER_H