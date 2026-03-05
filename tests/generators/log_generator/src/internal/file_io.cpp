#include "file_io.h"

#include <fstream>

auto ensure_directory(const std::filesystem::path& directory_path,
                      std::string& error_message) -> bool {
  try {
    std::filesystem::create_directories(directory_path);
  } catch (const std::filesystem::filesystem_error& error) {
    error_message = "Failed to create directory '" + directory_path.string() +
                    "': " + error.what();
    return false;
  }
  return true;
}

auto write_text_file(const std::filesystem::path& file_path,
                     const std::string& content,
                     std::string& error_message) -> bool {
  std::ofstream output_file(file_path);
  if (!output_file.is_open()) {
    error_message = "Cannot open output file '" + file_path.string() + "'.";
    return false;
  }
  output_file << content;
  return true;
}
