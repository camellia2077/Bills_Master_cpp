#include "reporting/report_render_service.hpp"

#include <stdexcept>

#include "reporting/renderers/standard_report_renderer_registry.hpp"
#include "reporting/standard_report/standard_report_assembler.hpp"

auto ReportRenderService::BuildStandardReport(const QueryExecutionResult& query_result)
    -> StandardReport {
  if (query_result.query_type == "year") {
    return StandardReportAssembler::FromYearly(query_result.yearly_data);
  }
  if (query_result.query_type == "month") {
    return StandardReportAssembler::FromMonthly(query_result.monthly_data);
  }
  throw std::invalid_argument("Unsupported query type for standard report rendering.");
}

auto ReportRenderService::Render(const StandardReport& report,
                                 std::string_view format_name) -> std::string {
  return StandardReportRendererRegistry::Render(report, format_name);
}
