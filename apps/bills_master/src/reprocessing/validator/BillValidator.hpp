// reprocessing/validator/BillValidator.hpp
#ifndef BILL_VALIDATOR_HPP
#define BILL_VALIDATOR_HPP

#include "reprocessing/validator/config/BillConfig.hpp"
#include "reprocessing/validator/result/ValidationResult.hpp"
#include <string>
#include <memory>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"

// --- NEW: TxtStructureVerifier class ---
// 只负责验证 TXT 的基本文件头结构
class TxtStructureVerifier {
public:
    bool verify(const std::string& bill_content, ValidationResult& result);
};


// --- NEW: JsonBillValidator class ---
// 负责从转换后的 JSON 对象中验证内容
class JsonBillValidator {
public:
    bool verify(const nlohmann::json& bill_json, const BillConfig& config, ValidationResult& result);
};


class BillValidator {
public:
    explicit BillValidator(const nlohmann::json& config_json);

    // --- MODIFIED: 方法被重构以支持新的两阶段验证流程 ---
    bool validate_txt_structure(const std::string& bill_content, ValidationResult& result);
    bool validate_json_content(const nlohmann::json& bill_json, ValidationResult& result);

private:
    std::unique_ptr<BillConfig> m_config;
    TxtStructureVerifier m_txt_verifier;
    JsonBillValidator m_json_verifier;
};

#endif // BILL_VALIDATOR_HPP