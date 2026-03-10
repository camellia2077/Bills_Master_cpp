#ifndef RECORD_TEMPLATE_FILE_SUPPORT_HPP_
#define RECORD_TEMPLATE_FILE_SUPPORT_HPP_

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "record_template/record_template_types.hpp"

class RecordTemplateFileSupport {
 public:
  [[nodiscard]] static auto ListFilesByExtension(
      const std::filesystem::path& input_path, std::string_view extension)
      -> RecordTemplateResult<std::vector<std::filesystem::path>>;

  [[nodiscard]] static auto ReadTextFile(
      const std::filesystem::path& file_path)
      -> RecordTemplateResult<std::string>;

  [[nodiscard]] static auto WriteTextFile(
      const std::filesystem::path& output_path, std::string_view text)
      -> RecordTemplateResult<std::filesystem::path>;
};

#endif  // RECORD_TEMPLATE_FILE_SUPPORT_HPP_
