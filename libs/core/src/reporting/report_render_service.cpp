#include "reporting/report_render_service.hpp"

#include "common/iso_period.hpp"
#include <stdexcept>

#include "reporting/renderers/standard_report_renderer_registry.hpp"
#include "reporting/standard_report/standard_report_assembler.hpp"

namespace {

auto BuildEmptyMonthlyProjection(const QueryExecutionResult& query_result)
    -> MonthlyReportData {
  MonthlyReportData report;
  if (const auto parsed = bills::core::common::iso_period::parse_year_month(
          query_result.period_start);
      parsed.has_value()) {
    report.year = parsed->year;
    report.month = parsed->month;
  }
  report.data_found = false;
  return report;
}

}  // namespace

auto ReportRenderService::BuildStandardReport(
    const QueryExecutionResult& query_result,
    const ReportPresentationKind presentation_kind)
    -> StandardReport {
  switch (presentation_kind) {
    case ReportPresentationKind::kMonthly:
      if (!query_result.range_data.months.empty()) {
        return StandardReportAssembler::FromMonthly(
            query_result.range_data.months.front());
      }
      return StandardReportAssembler::FromMonthly(
          BuildEmptyMonthlyProjection(query_result));
    case ReportPresentationKind::kYearly:
      return StandardReportAssembler::FromYearly(query_result.range_data);
    case ReportPresentationKind::kRange:
      return StandardReportAssembler::FromRange(query_result.range_data);
  }
  throw std::invalid_argument("Unsupported presentation kind for standard report rendering.");
}

auto ReportRenderService::Render(const StandardReport& report,
                                 std::string_view format_name) -> std::string {
  return StandardReportRendererRegistry::Render(report, format_name);
}
