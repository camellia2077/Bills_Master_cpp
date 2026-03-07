// billing/conversion/modifier/processor/bills_content_transformer.hpp

#ifndef BILLING_CONVERSION_MODIFIER_PROCESSOR_BILLS_CONTENT_TRANSFORMER_H_
#define BILLING_CONVERSION_MODIFIER_PROCESSOR_BILLS_CONTENT_TRANSFORMER_H_

#include <string>
#include <vector>

#include "domain/bill/bill_record.hpp"
#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"

/**
 * @class BillContentTransformer
 * @brief 协调将原始账单文本转换为结构化数据的整个流程。
 */
class BillContentTransformer {
 public:
  explicit BillContentTransformer(const Config& config);

  /**
   * @brief 执行完整的转换流程。
   * @param bill_content 原始的账单文件内容字符串。
   * @return 返回最终经过处理的账单领域数据。
   */
  ParsedBill process(const std::string& bill_content);

 private:
  const Config& m_config;

  // --- Private Static Helper Functions ---
  static std::vector<std::string> _split_string_by_lines(
      const std::string& str);
};

#endif  // BILLING_CONVERSION_MODIFIER_PROCESSOR_BILLS_CONTENT_TRANSFORMER_H_


