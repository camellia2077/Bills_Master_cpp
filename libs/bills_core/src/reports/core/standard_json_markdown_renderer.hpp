// reports/core/standard_json_markdown_renderer.hpp
#ifndef REPORTS_CORE_STANDARD_JSON_MARKDOWN_RENDERER_H_
#define REPORTS_CORE_STANDARD_JSON_MARKDOWN_RENDERER_H_

#include <string>

struct StandardReport;

class StandardJsonMarkdownRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
  static auto render(const std::string& standard_report_json) -> std::string;
};

#endif  // REPORTS_CORE_STANDARD_JSON_MARKDOWN_RENDERER_H_
