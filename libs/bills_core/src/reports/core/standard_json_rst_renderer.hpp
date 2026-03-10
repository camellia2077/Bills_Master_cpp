#ifndef REPORTS_CORE_STANDARD_JSON_RST_RENDERER_H_
#define REPORTS_CORE_STANDARD_JSON_RST_RENDERER_H_

#include <string>

struct StandardReport;

class StandardJsonRstRenderer {
 public:
  static auto render(const StandardReport& standard_report) -> std::string;
};

#endif  // REPORTS_CORE_STANDARD_JSON_RST_RENDERER_H_
