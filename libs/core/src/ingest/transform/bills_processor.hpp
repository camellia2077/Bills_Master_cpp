// ingest/transform/bills_processor.hpp

#ifndef INGEST_TRANSFORM_BILLS_PROCESSOR_H_
#define INGEST_TRANSFORM_BILLS_PROCESSOR_H_

#include <string>
#include <vector>

#include "config/modifier_data.hpp"

/**
 * @class BillProcessor
 * @brief 负责对原始账单文本行进行预处理。
 *
 * 包括与金额求值无关的预处理和自动续费规则。
 */
class BillProcessor {
 public:
  explicit BillProcessor(const Config& config);

  /**
   * @brief 执行所有不改写原始金额表达式的预处理。
   * @param lines 原始的文本行向量，将被就地修改。
   */
  void process(std::vector<std::string>& lines);

 private:
  const Config& m_config;

  void _apply_auto_renewal(std::vector<std::string>& lines);
  // is_title 是一个辅助函数，在 BillContentTransformer
  // 中也有，这里为了独立也保留一份
  static bool _is_title(const std::string& line);
};

#endif  // INGEST_TRANSFORM_BILLS_PROCESSOR_H_
