// reprocessing/Reprocessor.hpp
#ifndef REPROCESSOR_HPP
#define REPROCESSOR_HPP

#include <string>
#include <memory>
#include "reprocessing/validator/BillValidator.hpp"
#include "reprocessing/modifier/BillModifier.hpp"
#include "nlohmann/json.hpp" // 包含json头文件

class Reprocessor {
public:
    /**
     * @brief 构造函数，使用已验证的配置对象进行初始化。
     * @param validator_config_json 用于 BillValidator 的有效 JSON 配置。
     * @param modifier_config_json 用于 BillModifier 的有效 JSON 配置。
     */
    explicit Reprocessor(const nlohmann::json& validator_config_json, const nlohmann::json& modifier_config_json);

    bool validate_bill(const std::string& bill_path);
    bool modify_bill(const std::string& input_bill_path, const std::string& output_bill_path);

private:
    std::unique_ptr<BillValidator> m_validator;
    std::unique_ptr<BillModifier> m_modifier;
};

#endif // REPROCESSOR_HPP