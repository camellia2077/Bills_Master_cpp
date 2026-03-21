// reporting/renderers/standard_json_latex_renderer.hpp
#ifndef REPORTS_CORE_STANDARD_JSON_LATEX_RENDERER_H_
#define REPORTS_CORE_STANDARD_JSON_LATEX_RENDERER_H_

#include <string>

struct StandardReport;

class StandardJsonLatexRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
  static auto render(const std::string& standard_report_json) -> std::string;
};

#endif  // REPORTS_CORE_STANDARD_JSON_LATEX_RENDERER_H_
