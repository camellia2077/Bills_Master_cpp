// conversion/modifier/processor/BillParser.hpp

#ifndef BILL_PARSER_HPP
#define BILL_PARSER_HPP

#include <string>
#include <vector>

#include "common/structures/CommonData.hpp"
#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"

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
  static void _parse_content_line(const std::string& line, double& amount,
                                  std::string& description,
                                  std::string& comment);
  static double _get_numeric_value_from_content(
      const std::string& content_line);
};

#endif  // BILL_PARSER_HPP
