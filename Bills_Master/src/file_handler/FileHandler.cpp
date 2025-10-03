// file_handler/FileHandler.cpp

#include "FileHandler.hpp"
#include <stdexcept>
#include <iostream>

std::vector<fs::path> FileHandler::find_files_by_extension(const std::string& input_path_str, const std::string& extension) {
    std::vector<fs::path> files_to_process;
    fs::path input_path(input_path_str);

    if (!fs::exists(input_path)) {
        throw std::runtime_error("错误: 路径不存在: " + input_path_str);
    }

    if (fs::is_regular_file(input_path)) {
        if (input_path.extension() == extension) {
            files_to_process.push_back(input_path);
        } else {
            throw std::runtime_error("错误: 提供的文件不是一个 " + extension + " 文件。");
        }
    } else if (fs::is_directory(input_path)) {
        for (const auto& entry : fs::recursive_directory_iterator(input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                files_to_process.push_back(entry.path());
            }
        }
    } else {
        throw std::runtime_error("错误: 提供的路径不是一个常规文件或目录。");
    }

    if (files_to_process.empty()) {
        std::cout << "警告: 在指定路径下未找到任何 " << extension << " 文件。" << std::endl;
    }

    return files_to_process;
}