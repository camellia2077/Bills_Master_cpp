// config_validator/facade/ConfigValidator.hpp
#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include "nlohmann/json.hpp"
#include <string>

class ConfigValidator {
public:
    /**
     * @brief 验证 BillValidator 的 JSON 配置文件 (Validator_Config.json)。
     * @param config_json 要验证的 JSON 对象。
     * @param error_message 如果验证失败，用于存储错误信息的输出参数。
     * @return 如果配置有效，则返回 true，否则返回 false。
     */
    static bool validate_validator_config(const nlohmann::json& config_json, std::string& error_message);

    /**
     * @brief 验证 BillModifier 的 JSON 配置文件 (Modifier_Config.json)。
     * @param config_json 要验证的 JSON 对象。
     * @param error_message 如果验证失败，用于存储错误信息的输出参数。
     * @return 如果配置有效，则返回 true，否则返回 false。
     */
    static bool validate_modifier_config(const nlohmann::json& config_json, std::string& error_message);
};

#endif // CONFIG_VALIDATOR_HPP