#ifndef REPORTING_REPORT_RENDER_SERVICE_HPP_
#define REPORTING_REPORT_RENDER_SERVICE_HPP_

#include <string>

#include "query/query_service.hpp"
#include "reporting/standard_report/standard_report_dto.hpp"

class ReportRenderService {
 public:
  [[nodiscard]] static auto BuildStandardReport(const QueryExecutionResult& query_result)
      -> StandardReport;

  [[nodiscard]] static auto Render(const StandardReport& report,
                                   std::string_view format_name)
      -> std::string;
};

#endif  // REPORTING_REPORT_RENDER_SERVICE_HPP_
