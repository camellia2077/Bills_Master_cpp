#include "io/adapters/reports/report_export_service.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/iso_period.hpp"
#include "ports/report_data_gateway.hpp"
#include "query/query_service.hpp"
#include "reporting/renderers/standard_report_renderer_registry.hpp"

namespace {
namespace fs = std::filesystem;

auto extension_for_format(const std::string& format_name) -> std::string {
  if (format_name == "json") {
    return ".json";
  }
  return "." + format_name;
}

auto resolve_folder_name(
    const std::map<std::string, std::string>& format_folder_names,
    const std::string& normalized_format) -> std::string {
  const auto folder_it = format_folder_names.find(normalized_format);
  return folder_it == format_folder_names.end() ? normalized_format
                                                : folder_it->second;
}

}  // namespace

auto TryBuildReportExportYear(std::string_view raw)
    -> std::optional<ReportExportYear> {
  const auto parsed = bills::core::common::iso_period::parse_year(raw);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return ReportExportYear{
      .iso_year = std::string(raw),
      .year = *parsed,
  };
}

auto TryBuildReportExportMonth(std::string_view raw)
    -> std::optional<ReportExportMonth> {
  const auto parsed = bills::core::common::iso_period::parse_year_month(raw);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return ReportExportMonth{
      .iso_month = bills::core::common::iso_period::format_year_month(parsed->year,
                                                                      parsed->month),
      .year = parsed->year,
      .month = parsed->month,
  };
}

ReportExportService::ReportExportService(
    std::unique_ptr<ReportDataGateway> report_data_gateway,
    const std::string& export_base_dir,
    const std::map<std::string, std::string>& format_folder_names)
    : report_data_gateway_(std::move(report_data_gateway)),
      export_base_dir_(export_base_dir),
      format_folder_names_(format_folder_names) {
  if (report_data_gateway_ == nullptr) {
    throw std::invalid_argument("Report data gateway must not be null.");
  }
}

auto ReportExportService::ListAvailableFormats() -> std::vector<std::string> {
  return StandardReportRendererRegistry::ListAvailableFormats();
}

auto ReportExportService::list_normalized_available_months() const
    -> NormalizedAvailableMonths {
  NormalizedAvailableMonths result;
  for (const auto& month : report_data_gateway_->ListAvailableMonths()) {
    const auto normalized = TryBuildReportExportMonth(month);
    if (!normalized.has_value()) {
      result.had_invalid_entries = true;
      continue;
    }
    result.months.push_back(*normalized);
  }
  return result;
}

auto ReportExportService::write_report(const std::string& folder_name,
                                       const std::string& group_name,
                                       const std::string& stem,
                                       const std::string& extension,
                                       const std::string& content) const -> bool {
  const fs::path output_path =
      fs::path(export_base_dir_) / folder_name / group_name / (stem + extension);
  std::error_code create_error;
  fs::create_directories(output_path.parent_path(), create_error);
  if (create_error) {
    return false;
  }
  std::ofstream output(output_path, std::ios::binary);
  if (!output) {
    return false;
  }
  output << content;
  return output.good();
}

auto ReportExportService::write_standard_json(const std::string& group_name,
                                              const std::string& stem,
                                              const std::string& content) const
    -> bool {
  return write_report("standard_json", group_name, stem, ".json", content);
}

auto ReportExportService::write_rendered_report(
    const StandardReport& standard_report,
    std::string_view normalized_format, const std::string& group_name,
    const std::string& stem) const -> bool {
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    if (!write_standard_json(group_name, stem,
                             ReportRenderService::Render(standard_report, "json"))) {
      return false;
    }
  }
  const std::string report =
      ReportRenderService::Render(standard_report, normalized_format);
  return write_report(resolve_folder_name(format_folder_names_,
                                          std::string(normalized_format)),
                      group_name, stem,
                      extension_for_format(std::string(normalized_format)),
                      report);
}

auto ReportExportService::export_range_report(const ReportExportRange& range,
                                              const std::string& format_name,
                                              const ReportPresentationKind presentation_kind,
                                              const ReportExportOutputScope output_scope)
    -> ReportExportRunResult {
  // Range is the only export/query core model.
  // presentation_kind chooses the rendered view, while output_scope only
  // decides which output directory convention is used for the exported files.
  const std::string normalized_format =
      StandardReportRendererRegistry::NormalizeFormat(format_name);
  if (normalized_format.empty()) {
    return {.ok = false};
  }

  const auto query_result = QueryService::QueryRange(
      *report_data_gateway_, range.start.iso_month, range.end.iso_month);
  if (!query_result.data_found) {
    return {.ok = false};
  }

  const auto standard_report =
      ReportRenderService::BuildStandardReport(query_result, presentation_kind);

  std::string group_name;
  std::string stem;
  switch (output_scope) {
    case ReportExportOutputScope::kMonths:
      group_name = "months/" + std::to_string(range.start.year);
      stem = range.start.iso_month;
      break;
    case ReportExportOutputScope::kYears:
      group_name = "years";
      stem = std::to_string(range.start.year);
      break;
    case ReportExportOutputScope::kRanges:
      group_name = "ranges";
      stem = range.start.iso_month + "_" + range.end.iso_month;
      break;
  }

  if (!write_rendered_report(standard_report, normalized_format, group_name, stem)) {
    return {.ok = false};
  }
  return {.ok = true, .exported_count = 1U};
}

auto ReportExportService::export_all_monthly_reports(const std::string& format_name)
    -> ReportExportRunResult {
  const auto normalized_months = list_normalized_available_months();
  ReportExportRunResult result{
      .ok = !normalized_months.had_invalid_entries,
      .exported_count = 0U,
  };
  for (const auto& month : normalized_months.months) {
    const ReportExportRange range{.start = month, .end = month};
    const auto current_result = export_range_report(
        range, format_name, ReportPresentationKind::kMonthly,
        ReportExportOutputScope::kMonths);
    result.ok = current_result.ok && result.ok;
    result.exported_count += current_result.exported_count;
  }
  return result;
}

auto ReportExportService::export_all_yearly_reports(const std::string& format_name)
    -> ReportExportRunResult {
  std::set<int> years;
  const auto normalized_months = list_normalized_available_months();
  ReportExportRunResult result{
      .ok = !normalized_months.had_invalid_entries,
      .exported_count = 0U,
  };
  for (const auto& month : normalized_months.months) {
    years.insert(month.year);
  }
  for (const auto& year : years) {
    const ReportExportRange range{
        .start = ReportExportMonth{
            .iso_month = std::to_string(year) + "-01",
            .year = year,
            .month = 1,
        },
        .end = ReportExportMonth{
            .iso_month = std::to_string(year) + "-12",
            .year = year,
            .month = 12,
        },
    };
    const auto current_result = export_range_report(
        range, format_name, ReportPresentationKind::kYearly,
        ReportExportOutputScope::kYears);
    result.ok = current_result.ok && result.ok;
    result.exported_count += current_result.exported_count;
  }
  return result;
}

auto ReportExportService::export_all_reports(const std::string& format_name)
    -> ReportExportRunResult {
  const auto monthly_result = export_all_monthly_reports(format_name);
  const auto yearly_result = export_all_yearly_reports(format_name);
  return {
      .ok = monthly_result.ok && yearly_result.ok,
      .exported_count =
          monthly_result.exported_count + yearly_result.exported_count,
  };
}
