// conversion/validator/BillValidator.hpp
#ifndef BILL_VALIDATOR_HPP
#define BILL_VALIDATOR_HPP

#include <fstream>
#include <memory>
#include <regex>
#include <string>

#include "common/structures/CommonData.hpp"
#include "conversion/validator/config/BillConfig.hpp"
#include "conversion/validator/result/ValidationResult.hpp"

// --- NEW: TxtStructureVerifier class ---
// 只负责验证 TXT 的基本文件头结构
class TxtStructureVerifier {
 public:
  static bool verify(const std::string& bill_content, ValidationResult& result);
};

// --- NEW: BillContentValidator class ---
// 负责从领域账单数据中验证内容
class BillContentValidator {
 public:
  static bool verify(const ParsedBill& bill_data, const BillConfig& config,
                     ValidationResult& result);
};

class BillValidator {
 public:
  explicit BillValidator(BillConfig config);

  // --- MODIFIED: 方法被重构以支持新的两阶段验证流程 ---
  static bool validate_txt_structure(const std::string& bill_content,
                                     ValidationResult& result);
  bool validate_bill_content(const ParsedBill& bill_data,
                             ValidationResult& result);

 private:
  std::unique_ptr<BillConfig> m_config;
  TxtStructureVerifier m_txt_verifier;
  BillContentValidator m_bill_verifier;
};

#endif  // BILL_VALIDATOR_HPP
