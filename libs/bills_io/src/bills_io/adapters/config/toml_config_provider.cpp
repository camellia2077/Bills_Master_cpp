// adapters/config/TomlConfigProvider.cpp

#include "bills_io/adapters/config/toml_config_provider.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

#include <toml++/toml.hpp>

#include "bills_io/adapters/config/toml_bill_config_loader.hpp"
#include "bills_io/adapters/config/toml_modifier_config_loader.hpp"
#include "config_validator/pipeline/modifier_config_validator.hpp"
#include "config_validator/pipeline/validator_config_validator.hpp"

namespace fs = std::filesystem;

namespace {
const std::string kValidatorConfigName = "validator_config.toml";
const std::string kModifierConfigName = "modifier_config.toml";
const std::string kConfigContext = "ConfigProvider";

auto ReadTomlFile(const fs::path& file_path) -> toml::table {
  if (!fs::is_regular_file(file_path)) {
    throw std::runtime_error("Failed to open config file: " + file_path.string());
  }
  return toml::parse_file(file_path.string());
}
}  // namespace

auto TomlConfigProvider::Load(const std::string& config_path)
    -> Result<ConfigBundle> {
  try {
    const toml::table validator_toml =
        ReadTomlFile(fs::path(config_path) / kValidatorConfigName);

    std::string error_msg;
    if (!ValidatorConfigValidator::validate(validator_toml, error_msg)) {
      return std::unexpected(MakeError(
          kValidatorConfigName + " invalid: " + error_msg, kConfigContext));
    }
    BillConfig validator_config = TomlBillConfigLoader::Load(validator_toml);

    const toml::table modifier_toml =
        ReadTomlFile(fs::path(config_path) / kModifierConfigName);
    if (!ModifierConfigValidator::validate(modifier_toml, error_msg)) {
      return std::unexpected(MakeError(
          kModifierConfigName + " invalid: " + error_msg, kConfigContext));
    }
    Config modifier_config = TomlModifierConfigLoader::Load(modifier_toml);

    return ConfigBundle{.validator_config = std::move(validator_config),
                        .modifier_config = std::move(modifier_config)};
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kConfigContext));
  }
}
