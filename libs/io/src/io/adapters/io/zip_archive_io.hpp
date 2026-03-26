#ifndef BILLS_IO_ADAPTERS_IO_ZIP_ARCHIVE_IO_HPP_
#define BILLS_IO_ADAPTERS_IO_ZIP_ARCHIVE_IO_HPP_

#include <filesystem>
#include <string>
#include <vector>

#include "common/Result.hpp"

struct ZipArchiveTextEntry {
  std::string archive_path;
  std::string text;
};

class ZipArchiveIo {
 public:
  [[nodiscard]] static auto ReadTextEntries(
      const std::filesystem::path& archive_path)
      -> Result<std::vector<ZipArchiveTextEntry>>;

  [[nodiscard]] static auto WriteTextEntries(
      const std::filesystem::path& archive_path,
      const std::vector<ZipArchiveTextEntry>& entries) -> Result<void>;
};

#endif  // BILLS_IO_ADAPTERS_IO_ZIP_ARCHIVE_IO_HPP_
