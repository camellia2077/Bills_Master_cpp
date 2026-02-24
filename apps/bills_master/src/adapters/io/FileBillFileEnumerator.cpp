// adapters/io/FileBillFileEnumerator.cpp

#include "FileBillFileEnumerator.hpp"

FileBillFileEnumerator::FileBillFileEnumerator(FileHandler& file_handler)
    : file_handler_(file_handler) {}

auto FileBillFileEnumerator::ListFilesByExtension(std::string_view root_path,
                                                  std::string_view extension)
    -> std::vector<std::filesystem::path> {
  return FileHandler::find_files_by_extension(std::string(root_path),
                                              std::string(extension));
}
