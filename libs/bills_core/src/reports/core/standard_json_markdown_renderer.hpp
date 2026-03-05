#ifndef STANDARD_JSON_MARKDOWN_RENDERER_HPP
#define STANDARD_JSON_MARKDOWN_RENDERER_HPP

#include <string>

struct StandardReport;

class StandardJsonMarkdownRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
  static auto render(const std::string& standard_report_json) -> std::string;
};

#endif  // STANDARD_JSON_MARKDOWN_RENDERER_HPP
