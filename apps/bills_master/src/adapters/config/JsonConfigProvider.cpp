// adapters/config/JsonConfigProvider.cpp

#include "JsonConfigProvider.hpp"

#include <filesystem>

#include "adapters/config/JsonBillConfigLoader.hpp"
#include "adapters/config/JsonModifierConfigLoader.hpp"
#include "config_validator/facade/ConfigValidator.hpp"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace {
const std::string kValidatorConfigName = "Validator_Config.json";
const std::string kModifierConfigName = "Modifier_Config.json";
const std::string kConfigContext = "ConfigProvider";
}  // namespace

JsonConfigProvider::JsonConfigProvider(FileHandler& file_handler)
    : file_handler_(file_handler) {}

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
    if (!ConfigValidator::validate_validator_config(kValidatorJson,
                                                    error_msg)) {
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

    if (!ConfigValidator::validate_modifier_config(kModifierJson, error_msg)) {
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
