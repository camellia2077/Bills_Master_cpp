// bills_io/adapters/io/file_bill_file_enumerator.cpp
#include "bills_io/adapters/io/file_bill_file_enumerator.hpp"

#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

auto normalize_extension(std::string_view extension) -> std::string {
  if (extension.empty()) {
    return {};
  }
  if (extension.front() == '.') {
    return std::string(extension);
  }
  return "." + std::string(extension);
}

}  // namespace

auto FileBillFileEnumerator::ListFilesByExtension(std::string_view root_path,
                                                  std::string_view extension)
    -> std::vector<fs::path> {
  const fs::path input_path{std::string(root_path)};
  const std::string normalized_extension = normalize_extension(extension);

  if (normalized_extension.empty()) {
    throw std::runtime_error("错误: 文件后缀不能为空。");
  }
  if (!fs::exists(input_path)) {
    throw std::runtime_error("错误: 路径不存在: " + input_path.string());
  }

  std::vector<fs::path> files_to_process;
  if (fs::is_regular_file(input_path)) {
    if (input_path.extension() == normalized_extension) {
      files_to_process.push_back(input_path);
    } else {
      throw std::runtime_error("错误: 提供的文件不是一个 " +
                               normalized_extension + " 文件。");
    }
  } else if (fs::is_directory(input_path)) {
    for (const auto& entry : fs::recursive_directory_iterator(input_path)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() == normalized_extension) {
        files_to_process.push_back(entry.path());
      }
    }
  } else {
    throw std::runtime_error("错误: 提供的路径不是一个常规文件或目录。");
  }

  std::sort(files_to_process.begin(), files_to_process.end());
  return files_to_process;
}
