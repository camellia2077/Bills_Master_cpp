// windows/infrastructure/adapters/io/file_bill_file_enumerator.cpp

#include "file_bill_file_enumerator.hpp"

#include "windows/infrastructure/file_handler/file_handler.hpp"

auto FileBillFileEnumerator::ListFilesByExtension(std::string_view root_path,
                                                  std::string_view extension)
    -> std::vector<std::filesystem::path> {
  return FileHandler::find_files_by_extension(std::string(root_path),
                                              std::string(extension));
}
