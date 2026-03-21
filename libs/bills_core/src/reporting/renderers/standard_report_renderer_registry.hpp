#ifndef REPORTS_CORE_STANDARD_REPORT_RENDERER_REGISTRY_H_
#define REPORTS_CORE_STANDARD_REPORT_RENDERER_REGISTRY_H_

#include <string>
#include <string_view>
#include <vector>

struct StandardReport;

class StandardReportRendererRegistry {
 public:
  [[nodiscard]] static auto ListAvailableFormats() -> std::vector<std::string>;
  [[nodiscard]] static auto NormalizeFormat(std::string_view format_name)
      -> std::string;
  [[nodiscard]] static auto IsFormatAvailable(std::string_view format_name)
      -> bool;
  [[nodiscard]] static auto Render(const StandardReport& standard_report,
                                   std::string_view format_name)
      -> std::string;
};

#endif  // REPORTS_CORE_STANDARD_REPORT_RENDERER_REGISTRY_H_
