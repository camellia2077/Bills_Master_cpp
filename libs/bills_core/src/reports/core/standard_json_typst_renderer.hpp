#ifndef REPORTS_CORE_STANDARD_JSON_TYPST_RENDERER_H_
#define REPORTS_CORE_STANDARD_JSON_TYPST_RENDERER_H_

#include <string>

struct StandardReport;

class StandardJsonTypstRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
};

#endif  // REPORTS_CORE_STANDARD_JSON_TYPST_RENDERER_H_
