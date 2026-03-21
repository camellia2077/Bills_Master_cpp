#include "bills_io/adapters/io/source_document_io.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
constexpr const char* kContext = "SourceDocumentIo";

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

auto SourceDocumentIo::LoadByExtension(const std::filesystem::path& root_path,
                                       std::string_view extension)
    -> Result<SourceDocumentBatch> {
  const std::string normalized_extension = normalize_extension(extension);
  if (normalized_extension.empty()) {
    return std::unexpected(MakeError("File extension must not be empty.", kContext));
  }
  if (!std::filesystem::exists(root_path)) {
    return std::unexpected(
        MakeError("Path does not exist: " + root_path.string(), kContext));
  }

  SourceDocumentBatch documents;
  auto append_file = [&documents](const std::filesystem::path& file_path) -> Result<void> {
    const auto text = ReadText(file_path);
    if (!text) {
      return std::unexpected(text.error());
    }
    documents.push_back(SourceDocument{
        .display_path = file_path.string(),
        .text = *text,
    });
    return {};
  };

  if (std::filesystem::is_regular_file(root_path)) {
    if (root_path.extension() != normalized_extension) {
      return std::unexpected(MakeError(
          "Provided file does not match extension " + normalized_extension + ".",
          kContext));
    }
    const auto result = append_file(root_path);
    if (!result) {
      return std::unexpected(result.error());
    }
    return documents;
  }
  if (!std::filesystem::is_directory(root_path)) {
    return std::unexpected(
        MakeError("Path must be a file or directory: " + root_path.string(),
                  kContext));
  }

  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path)) {
    if (entry.is_regular_file() && entry.path().extension() == normalized_extension) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  for (const auto& file_path : files) {
    const auto result = append_file(file_path);
    if (!result) {
      return std::unexpected(result.error());
    }
  }
  return documents;
}

auto SourceDocumentIo::ReadText(const std::filesystem::path& file_path)
    -> Result<std::string> {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    return std::unexpected(
        MakeError("Failed to open file: " + file_path.string(), kContext));
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto SourceDocumentIo::WriteDocuments(const std::filesystem::path& output_root,
                                      const SourceDocumentBatch& documents)
    -> Result<std::vector<std::string>> {
  std::vector<std::string> written_paths;
  written_paths.reserve(documents.size());

  for (const auto& document : documents) {
    const std::filesystem::path output_path =
        output_root / std::filesystem::path(document.display_path);
    std::error_code create_error;
    std::filesystem::create_directories(output_path.parent_path(), create_error);
    if (create_error) {
      return std::unexpected(MakeError(
          "Failed to create directory: " + output_path.parent_path().string(),
          kContext));
    }
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
      return std::unexpected(
          MakeError("Failed to create file: " + output_path.string(), kContext));
    }
    output << document.text;
    if (!output.good()) {
      return std::unexpected(
          MakeError("Failed to write file: " + output_path.string(), kContext));
    }
    written_paths.push_back(output_path.string());
  }

  return written_paths;
}
