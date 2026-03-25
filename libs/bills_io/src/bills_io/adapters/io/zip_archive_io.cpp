#include "bills_io/adapters/io/zip_archive_io.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <sstream>
#include <string_view>

#include "miniz.h"

namespace {
constexpr const char* kContext = "ZipArchiveIo";

struct NormalizedArchivePath {
  std::string path;
  bool is_directory = false;
};

auto MakeZipError(std::string action, mz_zip_archive* archive) -> Error {
  const mz_zip_error error_code = mz_zip_get_last_error(archive);
  const char* error_message = mz_zip_get_error_string(error_code);
  std::string message = std::move(action);
  if (error_message != nullptr && error_message[0] != '\0') {
    message += ": ";
    message += error_message;
  }
  return MakeError(std::move(message), kContext);
}

auto NormalizeArchivePath(std::string raw_path)
    -> Result<NormalizedArchivePath> {
  if (raw_path.empty()) {
    return std::unexpected(MakeError("Archive entry path must not be empty.",
                                     kContext));
  }

  std::replace(raw_path.begin(), raw_path.end(), '\\', '/');
  bool is_directory = false;
  while (!raw_path.empty() && raw_path.back() == '/') {
    is_directory = true;
    raw_path.pop_back();
  }

  if (raw_path.empty()) {
    return std::unexpected(MakeError(
        "Archive entry path must not resolve to the root directory.", kContext));
  }
  if (raw_path.front() == '/') {
    return std::unexpected(MakeError(
        "Archive entry path must be relative: " + raw_path, kContext));
  }
  if (raw_path.size() >= 2U &&
      std::isalpha(static_cast<unsigned char>(raw_path[0])) != 0 &&
      raw_path[1] == ':') {
    return std::unexpected(MakeError(
        "Archive entry path must not contain a drive prefix: " + raw_path,
        kContext));
  }

  std::vector<std::string> segments;
  std::stringstream stream(raw_path);
  std::string segment;
  while (std::getline(stream, segment, '/')) {
    if (segment.empty()) {
      return std::unexpected(MakeError(
          "Archive entry path must not contain empty segments: " + raw_path,
          kContext));
    }
    if (segment == "." || segment == "..") {
      return std::unexpected(MakeError(
          "Archive entry path must not contain '.' or '..': " + raw_path,
          kContext));
    }
    segments.push_back(segment);
  }

  std::string normalized;
  for (std::size_t index = 0; index < segments.size(); ++index) {
    if (index > 0U) {
      normalized += '/';
    }
    normalized += segments[index];
  }
  return NormalizedArchivePath{
      .path = std::move(normalized),
      .is_directory = is_directory,
  };
}

auto EnsureParentDirectory(const std::filesystem::path& archive_path) -> Result<void> {
  if (!archive_path.has_parent_path()) {
    return {};
  }
  std::error_code create_error;
  std::filesystem::create_directories(archive_path.parent_path(), create_error);
  if (create_error) {
    return std::unexpected(MakeError(
        "Failed to create archive directory: " +
            archive_path.parent_path().string(),
        kContext));
  }
  return {};
}
}  // namespace

auto ZipArchiveIo::ReadTextEntries(const std::filesystem::path& archive_path)
    -> Result<std::vector<ZipArchiveTextEntry>> {
  if (!std::filesystem::exists(archive_path)) {
    return std::unexpected(MakeError(
        "Archive path does not exist: " + archive_path.string(), kContext));
  }

  mz_zip_archive archive{};
  mz_zip_zero_struct(&archive);
  if (!mz_zip_reader_init_file(&archive, archive_path.string().c_str(), 0U)) {
    return std::unexpected(
        MakeZipError("Failed to open ZIP archive '" + archive_path.string() + "'",
                     &archive));
  }

  std::vector<ZipArchiveTextEntry> entries;
  std::set<std::string> seen_paths;
  const mz_uint file_count = mz_zip_reader_get_num_files(&archive);
  for (mz_uint index = 0U; index < file_count; ++index) {
    mz_zip_archive_file_stat file_stat{};
    if (!mz_zip_reader_file_stat(&archive, index, &file_stat)) {
      const Error error = MakeZipError("Failed to inspect archive entry", &archive);
      mz_zip_reader_end(&archive);
      return std::unexpected(error);
    }

    auto normalized_path = NormalizeArchivePath(file_stat.m_filename);
    if (!normalized_path) {
      mz_zip_reader_end(&archive);
      return std::unexpected(normalized_path.error());
    }
    if (file_stat.m_is_directory || normalized_path->is_directory) {
      continue;
    }
    if (!seen_paths.insert(normalized_path->path).second) {
      mz_zip_reader_end(&archive);
      return std::unexpected(MakeError(
          "Archive contains duplicate entry: " + normalized_path->path,
          kContext));
    }

    size_t extracted_size = 0U;
    void* raw_text =
        mz_zip_reader_extract_to_heap(&archive, index, &extracted_size, 0U);
    if (raw_text == nullptr) {
      const Error error = MakeZipError(
          "Failed to extract archive entry '" + normalized_path->path + "'",
          &archive);
      mz_zip_reader_end(&archive);
      return std::unexpected(error);
    }

    const char* text_data = static_cast<const char*>(raw_text);
    entries.push_back(ZipArchiveTextEntry{
        .archive_path = normalized_path->path,
        .text = std::string(text_data, extracted_size),
    });
    mz_free(raw_text);
  }

  if (!mz_zip_reader_end(&archive)) {
    return std::unexpected(
        MakeZipError("Failed to close ZIP archive '" + archive_path.string() + "'",
                     &archive));
  }
  return entries;
}

auto ZipArchiveIo::WriteTextEntries(
    const std::filesystem::path& archive_path,
    const std::vector<ZipArchiveTextEntry>& entries) -> Result<void> {
  const auto ensure_parent = EnsureParentDirectory(archive_path);
  if (!ensure_parent) {
    return std::unexpected(ensure_parent.error());
  }

  mz_zip_archive archive{};
  mz_zip_zero_struct(&archive);
  if (!mz_zip_writer_init_file(&archive, archive_path.string().c_str(), 0U)) {
    return std::unexpected(
        MakeZipError("Failed to create ZIP archive '" + archive_path.string() + "'",
                     &archive));
  }

  bool should_cleanup = true;
  std::set<std::string> seen_paths;
  for (const auto& entry : entries) {
    auto normalized_path = NormalizeArchivePath(entry.archive_path);
    if (!normalized_path) {
      mz_zip_writer_end(&archive);
      std::error_code cleanup_error;
      std::filesystem::remove(archive_path, cleanup_error);
      return std::unexpected(normalized_path.error());
    }
    if (normalized_path->is_directory) {
      mz_zip_writer_end(&archive);
      std::error_code cleanup_error;
      std::filesystem::remove(archive_path, cleanup_error);
      return std::unexpected(MakeError(
          "ZIP writer only accepts file entries: " + normalized_path->path,
          kContext));
    }
    if (!seen_paths.insert(normalized_path->path).second) {
      mz_zip_writer_end(&archive);
      std::error_code cleanup_error;
      std::filesystem::remove(archive_path, cleanup_error);
      return std::unexpected(MakeError(
          "ZIP archive contains duplicate entry path: " + normalized_path->path,
          kContext));
    }

    if (!mz_zip_writer_add_mem(&archive, normalized_path->path.c_str(),
                               entry.text.data(), entry.text.size(),
                               MZ_BEST_COMPRESSION)) {
      const Error error = MakeZipError(
          "Failed to write archive entry '" + normalized_path->path + "'",
          &archive);
      mz_zip_writer_end(&archive);
      std::error_code cleanup_error;
      std::filesystem::remove(archive_path, cleanup_error);
      return std::unexpected(error);
    }
  }

  if (!mz_zip_writer_finalize_archive(&archive)) {
    const Error error = MakeZipError(
        "Failed to finalize ZIP archive '" + archive_path.string() + "'", &archive);
    mz_zip_writer_end(&archive);
    std::error_code cleanup_error;
    std::filesystem::remove(archive_path, cleanup_error);
    return std::unexpected(error);
  }
  should_cleanup = false;

  if (!mz_zip_writer_end(&archive)) {
    const Error error = MakeZipError(
        "Failed to close ZIP archive '" + archive_path.string() + "'", &archive);
    if (should_cleanup) {
      std::error_code cleanup_error;
      std::filesystem::remove(archive_path, cleanup_error);
    }
    return std::unexpected(error);
  }
  return {};
}
