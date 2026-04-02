// ingest/transform/bills_parser.hpp

#ifndef INGEST_TRANSFORM_BILLS_PARSER_H_
#define INGEST_TRANSFORM_BILLS_PARSER_H_

#include <string>
#include <vector>

#include "domain/bill/bill_record.hpp"
#include "config/modifier_data.hpp"

/**
 * @class BillParser
 * @brief 负责将预处理过的文本行解析成领域账单数据。
 */
class BillParser {
 public:
  explicit BillParser(const Config& config);

  /**
   * @brief 执行解析。
   * @param lines 预处理过的文本行。
   * @return 返回领域账单数据。
   */
  ParsedBill parse(const std::vector<std::string>& lines) const;

 private:
  const Config& m_config;

  bool _is_metadata_line(const std::string& line) const;
  static bool _is_parent_title(const std::string& line);
  static bool _is_title(const std::string& line);
  static std::string& _trim(std::string& text);
  static double _evaluate_amount_expression(const std::string& parent_category,
                                            const std::string& math_expr);
  static void _parse_content_line(const std::string& parent_category,
                                  const std::string& line, double& amount,
                                  std::string& description,
                                  std::string& comment);
  static double _get_numeric_value_from_content(
      const std::string& parent_category,
      const std::string& content_line);
};

#endif  // INGEST_TRANSFORM_BILLS_PARSER_H_

