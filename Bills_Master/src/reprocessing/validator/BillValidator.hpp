// reprocessing/validator/BillValidator.hpp
#ifndef BILL_VALIDATOR_HPP
#define BILL_VALIDATOR_HPP

#include "reprocessing/validator/config/BillConfig.hpp"
#include "reprocessing/validator/verifier/BillFormatVerifier.hpp"
#include "reprocessing/validator/result/ValidationResult.hpp"
#include <string>
#include <memory>
#include "nlohmann/json.hpp" // 包含json头文件

class BillValidator {
public:
    /**
     * @brief 构造函数，使用一个已验证的 JSON 对象来初始化。
     * @param config_json 包含验证器配置的 nlohmann::json 对象。
     */
    explicit BillValidator(const nlohmann::json& config_json);

    bool validate(const std::string& bill_file_path);

private:
    std::unique_ptr<BillConfig> config;
    BillFormatVerifier processor;
    ValidationResult result;
};

#endif // BILL_VALIDATOR_HPP