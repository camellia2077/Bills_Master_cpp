#ifndef REPORTING_REPORT_RENDER_SERVICE_HPP_
#define REPORTING_REPORT_RENDER_SERVICE_HPP_

#include <string>

#include "query/query_service.hpp"
#include "reporting/standard_report/standard_report_dto.hpp"

enum class ReportPresentationKind {
  kMonthly,
  kYearly,
  kRange,
};

class ReportRenderService {
 public:
  // presentation_kind only selects how one range query result is presented.
  // It does not imply different query paths or different underlying data
  // contracts in the reporting core.
  [[nodiscard]] static auto BuildStandardReport(
      const QueryExecutionResult& query_result,
      ReportPresentationKind presentation_kind)
      -> StandardReport;

  [[nodiscard]] static auto Render(const StandardReport& report,
                                   std::string_view format_name)
      -> std::string;
};

#endif  // REPORTING_REPORT_RENDER_SERVICE_HPP_
