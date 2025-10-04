// app_controller/ConfigLoader.cpp
#include "ConfigLoader.hpp"
#include "config_validator/facade/ConfigValidator.hpp"
#include "file_handler/FileHandler.hpp" // 包含 FileHandler
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

nlohmann::json ConfigLoader::load_and_validate_validator_config(const std::string& config_path, FileHandler& file_handler) {
    const fs::path validator_config_path = fs::path(config_path) / "Validator_Config.json";
    
    // 使用 FileHandler 读取文件
    const std::string file_content = file_handler.read_text_file(validator_config_path);
    nlohmann::json validator_config = nlohmann::json::parse(file_content);

    std::string error_msg;
    if (!ConfigValidator::validate_validator_config(validator_config, error_msg)) {
        throw std::runtime_error("Validator_Config.json 无效: " + error_msg);
    }
    return validator_config;
}

nlohmann::json ConfigLoader::load_and_validate_modifier_config(const std::string& config_path, FileHandler& file_handler) {
    const fs::path modifier_config_path = fs::path(config_path) / "Modifier_Config.json";

    // 使用 FileHandler 读取文件
    const std::string file_content = file_handler.read_text_file(modifier_config_path);
    nlohmann::json modifier_config = nlohmann::json::parse(file_content);

    std::string error_msg;
    if (!ConfigValidator::validate_modifier_config(modifier_config, error_msg)) {
        throw std::runtime_error("Modifier_Config.json 无效: " + error_msg);
    }
    return modifier_config;
}