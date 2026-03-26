#include "io/adapters/io/source_document_io.hpp"

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

template <typename DisplayPathBuilder>
auto load_documents_by_extension(const std::filesystem::path& root_path,
                                 std::string_view extension,
                                 DisplayPathBuilder&& build_display_path)
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
  auto append_file = [&documents, &build_display_path](
                         const std::filesystem::path& file_path) -> Result<void> {
    const auto text = SourceDocumentIo::ReadText(file_path);
    if (!text) {
      return std::unexpected(text.error());
    }
    const auto display_path = build_display_path(file_path);
    if (!display_path) {
      return std::unexpected(display_path.error());
    }
    documents.push_back(SourceDocument{
        .display_path = *display_path,
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
}  // namespace

auto SourceDocumentIo::LoadByExtension(const std::filesystem::path& root_path,
                                       std::string_view extension)
    -> Result<SourceDocumentBatch> {
  return load_documents_by_extension(
      root_path, extension,
      [](const std::filesystem::path& file_path) -> Result<std::string> {
        return file_path.string();
      });
}

auto SourceDocumentIo::LoadByExtensionRelative(
    const std::filesystem::path& root_path, std::string_view extension)
    -> Result<SourceDocumentBatch> {
  const std::filesystem::path normalized_root = root_path.lexically_normal();
  return load_documents_by_extension(
      normalized_root, extension,
      [normalized_root](const std::filesystem::path& file_path) -> Result<std::string> {
        if (std::filesystem::is_regular_file(normalized_root)) {
          return file_path.filename().generic_string();
        }
        const std::filesystem::path relative_path =
            file_path.lexically_relative(normalized_root);
        if (relative_path.empty() || relative_path == ".") {
          return std::unexpected(MakeError(
              "Failed to resolve relative path for: " + file_path.string(),
              kContext));
        }
        return relative_path.generic_string();
      });
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

auto SourceDocumentIo::WriteText(const std::filesystem::path& file_path,
                                 std::string_view text) -> Result<void> {
  std::error_code create_error;
  if (file_path.has_parent_path()) {
    std::filesystem::create_directories(file_path.parent_path(), create_error);
    if (create_error) {
      return std::unexpected(MakeError(
          "Failed to create directory: " + file_path.parent_path().string(),
          kContext));
    }
  }

  std::ofstream output(file_path, std::ios::binary);
  if (!output) {
    return std::unexpected(
        MakeError("Failed to create file: " + file_path.string(), kContext));
  }
  output.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!output.good()) {
    return std::unexpected(
        MakeError("Failed to write file: " + file_path.string(), kContext));
  }
  return {};
}

auto SourceDocumentIo::WriteDocuments(const std::filesystem::path& output_root,
                                      const SourceDocumentBatch& documents)
    -> Result<std::vector<std::string>> {
  std::vector<std::string> written_paths;
  written_paths.reserve(documents.size());

  for (const auto& document : documents) {
    const std::filesystem::path output_path =
        output_root / std::filesystem::path(document.display_path);
    const auto write_result = WriteText(output_path, document.text);
    if (!write_result) {
      return std::unexpected(write_result.error());
    }
    written_paths.push_back(output_path.string());
  }

  return written_paths;
}
