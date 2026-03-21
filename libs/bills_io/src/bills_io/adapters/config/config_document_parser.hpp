#ifndef BILLS_IO_ADAPTERS_CONFIG_CONFIG_DOCUMENT_PARSER_HPP_
#define BILLS_IO_ADAPTERS_CONFIG_CONFIG_DOCUMENT_PARSER_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include "common/Result.hpp"
#include "config/config_document_types.hpp"

class ConfigDocumentParser {
 public:
  [[nodiscard]] static auto ParseFiles(
      const std::filesystem::path& validator_config_path,
      const std::filesystem::path& modifier_config_path,
      const std::filesystem::path& export_formats_path)
      -> Result<ConfigDocumentBundle>;

  [[nodiscard]] static auto ParseTexts(
      std::string_view validator_config_text,
      std::string_view modifier_config_text,
      std::string_view export_formats_text,
      const std::string& validator_display_path = "validator_config.toml",
      const std::string& modifier_display_path = "modifier_config.toml",
      const std::string& export_formats_display_path = "export_formats.toml")
      -> ConfigDocumentBundle;
};

#endif  // BILLS_IO_ADAPTERS_CONFIG_CONFIG_DOCUMENT_PARSER_HPP_
