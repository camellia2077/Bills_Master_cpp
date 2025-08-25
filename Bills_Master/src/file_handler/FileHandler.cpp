
// FileHandler.cpp

#include "FileHandler.h"  // 包含对应的头文件，以获取类的声明
#include <stdexcept>      // 引入 std::runtime_error 等标准异常类
#include <iostream>       // 引入 std::cout 和 std::endl，用于向控制台打印警告信息

/**
 * @brief find_txt_files 方法的具体实现。
 * @param input_path_str 用户提供的路径字符串。
 * @return 包含所有 .txt 文件路径的向量。
 */
std::vector<fs::path> FileHandler::find_txt_files(const std::string& input_path_str) {
    // 创建一个向量，用于存储最终需要处理的文件路径
    std::vector<fs::path> files_to_process;
    
    // 将输入的字符串路径转换为 filesystem::path 对象，方便进行文件系统操作
    fs::path input_path(input_path_str);

    // --- 1. 验证路径是否存在 ---
    // 这是最基本的检查，如果路径本身就不存在，则无法继续处理
    if (!fs::exists(input_path)) {
        // 抛出一个运行时错误，并附带错误信息
        throw std::runtime_error("错误: 路径不存在: " + input_path_str);
    }

    // --- 2. 判断路径类型并处理 ---
    // a) 如果路径指向一个常规文件
    if (fs::is_regular_file(input_path)) {
        // 检查文件的扩展名是否为 ".txt"
        if (input_path.extension() == ".txt") {
            // 如果是，则将其添加到待处理文件列表中
            files_to_process.push_back(input_path);
        } else {
            // 如果不是 .txt 文件，则根据设计抛出错误
            throw std::runtime_error("错误: 提供的文件不是一个 .txt 文件。");
        }
    // b) 如果路径指向一个目录
    } else if (fs::is_directory(input_path)) {
        // 使用递归目录迭代器 (recursive_directory_iterator) 遍历目录及其所有子目录
        for (const auto& entry : fs::recursive_directory_iterator(input_path)) {
            // 对遍历到的每个条目，检查它是否是一个常规文件并且扩展名是 ".txt"
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                // 如果满足条件，将其路径添加到列表中
                files_to_process.push_back(entry.path());
            }
        }
    // c) 如果路径既不是常规文件也不是目录（例如是一个符号链接、套接字等）
    } else {
        // 抛出错误，因为本程序只设计处理文件和目录
        throw std::runtime_error("错误: 提供的路径不是一个常规文件或目录。");
    }

    // --- 3. 检查处理结果 ---
    // 在所有处理完成后，检查列表是否为空
    if (files_to_process.empty()) {
        // 如果没有找到任何 .txt 文件，向用户打印一条警告信息
        // 这比静默返回一个空列表要友好，能让用户知道程序确实运行了但没找到符合条件的文件
        std::cout << "警告: 在指定路径下未找到任何 .txt 文件。" << std::endl;
    }

    // 返回包含所有找到的 .txt 文件路径的向量
    return files_to_process;
}