// conversion/modifier/processor/BillContentTransformer.cpp

#include "BillContentTransformer.hpp"

#include <sstream>

#include "BillParser.hpp"     // --- 引入新的解析器 ---
#include "BillProcessor.hpp"  // --- 引入新的预处理器 ---

BillContentTransformer::BillContentTransformer(const Config& config)
    : m_config(config) {}

auto BillContentTransformer::process(const std::string& bill_content)
    -> ParsedBill {
  // 1. 将原始字符串按行分割
  std::vector<std::string> lines = _split_string_by_lines(bill_content);

  // 2. 使用 BillProcessor 对文本行进行预处理
  BillProcessor preprocessor(m_config);
  preprocessor.process(lines);

  // 3. 使用 BillParser 将处理后的行解析为结构化数据
  BillParser parser(m_config);
  return parser.parse(lines);
}

auto BillContentTransformer::_split_string_by_lines(const std::string& str)
    -> std::vector<std::string> {
  std::vector<std::string> lines;
  std::string line;
  std::istringstream stream(str);
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}
