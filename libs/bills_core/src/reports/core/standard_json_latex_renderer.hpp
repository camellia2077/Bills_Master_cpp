#ifndef STANDARD_JSON_LATEX_RENDERER_HPP
#define STANDARD_JSON_LATEX_RENDERER_HPP

#include <string>

struct StandardReport;

class StandardJsonLatexRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
  static auto render(const std::string& standard_report_json) -> std::string;
};

#endif  // STANDARD_JSON_LATEX_RENDERER_HPP
