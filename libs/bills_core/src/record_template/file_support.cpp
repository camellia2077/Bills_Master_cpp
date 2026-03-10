#include "record_template/file_support.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

auto RecordTemplateFileSupport::ListFilesByExtension(
    const std::filesystem::path& input_path, std::string_view extension)
    -> RecordTemplateResult<std::vector<std::filesystem::path>> {
  if (!std::filesystem::exists(input_path)) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kInputPath,
        "input_path does not exist: " + input_path.string()));
  }

  std::vector<std::filesystem::path> files;
  if (std::filesystem::is_regular_file(input_path)) {
    if (input_path.extension() != extension) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kInputPath,
          "input_path must be a " + std::string(extension) +
              " file or a directory: " + input_path.string()));
    }
    files.push_back(input_path);
  } else if (std::filesystem::is_directory(input_path)) {
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(input_path)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() == extension) {
        files.push_back(entry.path());
      }
    }
  } else {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kInputPath,
        "input_path must be a file or directory: " + input_path.string()));
  }

  std::ranges::sort(files);
  return files;
}

auto RecordTemplateFileSupport::ReadTextFile(
    const std::filesystem::path& file_path)
    -> RecordTemplateResult<std::string> {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kInputPath,
        "Failed to open file: " + file_path.string()));
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto RecordTemplateFileSupport::WriteTextFile(
    const std::filesystem::path& output_path, std::string_view text)
    -> RecordTemplateResult<std::filesystem::path> {
  try {
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kOutputPath,
          "Failed to create file: " + output_path.string()));
    }
    output << text;
    if (!output.good()) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kOutputPath,
          "Failed to write file: " + output_path.string()));
    }
    return output_path;
  } catch (const std::exception& error) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kOutputPath, error.what()));
  }
}
