// config_validator/facade/ConfigValidator.cpp
#include "ConfigValidator.hpp"
#include "config_validator/pipeline/ValidatorConfigValidator.hpp"
#include "config_validator/pipeline/ModifierConfigValidator.hpp"

bool ConfigValidator::validate_validator_config(const nlohmann::json& config_json, std::string& error_message) {
    // 将调用委托给专门的验证类
    return ValidatorConfigValidator::validate(config_json, error_message);
}

bool ConfigValidator::validate_modifier_config(const nlohmann::json& config_json, std::string& error_message) {
    // 将调用委托给专门的验证类
    return ModifierConfigValidator::validate(config_json, error_message);
}