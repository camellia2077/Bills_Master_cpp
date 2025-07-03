#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class FileHandler {
public:
    FileHandler() = default;

    /**
     * @brief Finds all .txt files at a given path.
     * * This function can handle a path to a single .txt file or a path to a directory.
     * If a directory is provided, it will be searched recursively for all .txt files.
     * * @param input_path_str The user-provided path string.
     * @return A vector of filesystem::path objects for each .txt file found.
     * @throws std::runtime_error if the path is invalid or if a single file is not a .txt file.
     */
    std::vector<fs::path> find_txt_files(const std::string& input_path_str);
};

#endif // FILE_HANDLER_H