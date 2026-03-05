// adapters/config/JsonConfigProvider.cpp

#include "json_config_provider.hpp"

#include <filesystem>

#include "windows/infrastructure/adapters/config/json_bill_config_loader.hpp"
#include "windows/infrastructure/adapters/config/json_modifier_config_loader.hpp"
#include "windows/infrastructure/file_handler/file_handler.hpp"
#include "config_validator/pipeline/modifier_config_validator.hpp"
#include "config_validator/pipeline/validator_config_validator.hpp"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace {
const std::string kValidatorConfigName = "Validator_Config.json";
const std::string kModifierConfigName = "Modifier_Config.json";
const std::string kConfigContext = "ConfigProvider";
}  // namespace

auto JsonConfigProvider::Load(const std::string& config_path)
    -> Result<ConfigBundle> {
  try {
    const fs::path kValidatorConfigPath =
        fs::path(config_path) / kValidatorConfigName;
    const std::string kValidatorContent =
        FileHandler::read_text_file(kValidatorConfigPath);
    const nlohmann::json kValidatorJson =
        nlohmann::json::parse(kValidatorContent);

    std::string error_msg;
    if (!ValidatorConfigValidator::validate(kValidatorJson, error_msg)) {
      return std::unexpected(MakeError(
          kValidatorConfigName + " 无效: " + error_msg, kConfigContext));
    }
    BillConfig validator_config = JsonBillConfigLoader::Load(kValidatorJson);

    const fs::path kModifierConfigPath =
        fs::path(config_path) / kModifierConfigName;
    const std::string kModifierContent =
        FileHandler::read_text_file(kModifierConfigPath);
    const nlohmann::json kModifierJson =
        nlohmann::json::parse(kModifierContent);

    if (!ModifierConfigValidator::validate(kModifierJson, error_msg)) {
      return std::unexpected(MakeError(
          kModifierConfigName + " 无效: " + error_msg, kConfigContext));
    }
    Config modifier_config = JsonModifierConfigLoader::Load(kModifierJson);

    return ConfigBundle{.validator_config = std::move(validator_config),
                        .modifier_config = std::move(modifier_config)};
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kConfigContext));
  }
}


