// ingest/validation/bills_validator.hpp
#ifndef INGEST_VALIDATION_BILLS_VALIDATOR_H_
#define INGEST_VALIDATION_BILLS_VALIDATOR_H_

#include <memory>
#include <string>

#include "ingest/validation/bills_config.hpp"
#include "ingest/validation/validation_result.hpp"
#include "domain/bill/bill_record.hpp"

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
};

#endif  // INGEST_VALIDATION_BILLS_VALIDATOR_H_

