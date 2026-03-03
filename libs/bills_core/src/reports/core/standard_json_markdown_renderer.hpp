#ifndef STANDARD_JSON_MARKDOWN_RENDERER_HPP
#define STANDARD_JSON_MARKDOWN_RENDERER_HPP

#include <string>

class StandardJsonMarkdownRenderer {
 public:
  static auto render(const std::string& standard_report_json) -> std::string;
};

#endif  // STANDARD_JSON_MARKDOWN_RENDERER_HPP
