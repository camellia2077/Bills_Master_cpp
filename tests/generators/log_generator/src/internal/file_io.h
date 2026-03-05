#ifndef FILE_IO_H
#define FILE_IO_H

#include <filesystem>
#include <string>

auto ensure_directory(const std::filesystem::path& directory_path,
                      std::string& error_message) -> bool;

auto write_text_file(const std::filesystem::path& file_path,
                     const std::string& content,
                     std::string& error_message) -> bool;

#endif  // FILE_IO_H
