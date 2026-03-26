#ifndef BILLS_IO_ADAPTERS_IO_SOURCE_DOCUMENT_IO_HPP_
#define BILLS_IO_ADAPTERS_IO_SOURCE_DOCUMENT_IO_HPP_

#include <filesystem>
#include <string_view>
#include <vector>

#include "common/Result.hpp"
#include "common/source_document.hpp"

class SourceDocumentIo {
 public:
  [[nodiscard]] static auto LoadByExtension(const std::filesystem::path& root_path,
                                            std::string_view extension)
      -> Result<SourceDocumentBatch>;

  [[nodiscard]] static auto LoadByExtensionRelative(
      const std::filesystem::path& root_path, std::string_view extension)
      -> Result<SourceDocumentBatch>;

  [[nodiscard]] static auto ReadText(const std::filesystem::path& file_path)
      -> Result<std::string>;

  [[nodiscard]] static auto WriteText(const std::filesystem::path& file_path,
                                      std::string_view text) -> Result<void>;

  [[nodiscard]] static auto WriteDocuments(
      const std::filesystem::path& output_root,
      const SourceDocumentBatch& documents) -> Result<std::vector<std::string>>;
};

#endif  // BILLS_IO_ADAPTERS_IO_SOURCE_DOCUMENT_IO_HPP_
