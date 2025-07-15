#include "FileHandler.h"
#include <stdexcept>
#include <iostream>

std::vector<fs::path> FileHandler::find_txt_files(const std::string& input_path_str) {
    std::vector<fs::path> files_to_process;
    fs::path input_path(input_path_str);

    if (!fs::exists(input_path)) {
        throw std::runtime_error("Error: Path does not exist: " + input_path_str);
    }

    if (fs::is_regular_file(input_path)) {
        if (input_path.extension() == ".txt") {
            files_to_process.push_back(input_path);
        } else {
            throw std::runtime_error("Error: The provided file is not a .txt file.");
        }
    } else if (fs::is_directory(input_path)) {
        for (const auto& entry : fs::recursive_directory_iterator(input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                files_to_process.push_back(entry.path());
            }
        }
    } else {
        throw std::runtime_error("Error: The provided path is not a regular file or directory.");
    }

    if (files_to_process.empty()) {
        std::cout << "Warning: No .txt files found at the specified path." << std::endl;
    }

    return files_to_process;
}