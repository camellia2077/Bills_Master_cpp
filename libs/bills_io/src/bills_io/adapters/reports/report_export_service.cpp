#include "bills_io/adapters/reports/report_export_service.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "ports/report_data_gateway.hpp"
#include "query/query_service.hpp"
#include "reporting/renderers/standard_report_renderer_registry.hpp"
#include "reporting/report_render_service.hpp"

namespace {
namespace fs = std::filesystem;

constexpr std::size_t kYearLength = 4U;
constexpr int kMinSupportedYear = 1900;
constexpr int kMaxSupportedYear = 9999;
const std::regex kIsoYearRegex(R"(^\d{4}$)");
const std::regex kIsoMonthRegex(R"(^(\d{4})-(0[1-9]|1[0-2])$)");

auto parse_iso_year(const std::string& value, int& year) -> bool {
  if (!std::regex_match(value, kIsoYearRegex)) {
    return false;
  }
  try {
    year = std::stoi(value);
  } catch (...) {
    return false;
  }
  return year >= kMinSupportedYear && year <= kMaxSupportedYear;
}

auto parse_iso_month(const std::string& value, int& year, int& month) -> bool {
  std::smatch match;
  if (!std::regex_match(value, match, kIsoMonthRegex) || match.size() != 3U) {
    return false;
  }
  try {
    year = std::stoi(match[1].str());
    month = std::stoi(match[2].str());
  } catch (...) {
    return false;
  }
  return year >= kMinSupportedYear && year <= kMaxSupportedYear;
}

auto month_key(int year, int month) -> int { return year * 100 + month; }

auto extension_for_format(const std::string& format_name) -> std::string {
  if (format_name == "json") {
    return ".json";
  }
  return "." + format_name;
}

}  // namespace

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

auto ReportExportService::export_yearly_report(const std::string& year_str,
                                               const std::string& format_name,
                                               bool suppress_output) -> bool {
  int year = 0;
  if (!parse_iso_year(year_str, year)) {
    return false;
  }
  const std::string normalized_format =
      StandardReportRendererRegistry::NormalizeFormat(format_name);
  if (normalized_format.empty()) {
    return false;
  }

  const auto query_result = QueryService::QueryYear(*report_data_gateway_, year_str);
  if (!query_result.data_found) {
    return false;
  }

  const auto standard_report = ReportRenderService::BuildStandardReport(query_result);
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    if (!write_standard_json("years", year_str,
                             ReportRenderService::Render(standard_report, "json"))) {
      return false;
    }
  }
  const std::string report = ReportRenderService::Render(standard_report, normalized_format);
  if (!suppress_output) {
    std::cout << report;
  }
  const auto folder_it = format_folder_names_.find(normalized_format);
  const std::string folder_name =
      folder_it == format_folder_names_.end() ? normalized_format : folder_it->second;
  return write_report(folder_name, "years", year_str,
                      extension_for_format(normalized_format), report);
}

auto ReportExportService::export_monthly_report(const std::string& month_str,
                                                const std::string& format_name,
                                                bool suppress_output) -> bool {
  int year = 0;
  int month = 0;
  if (!parse_iso_month(month_str, year, month)) {
    return false;
  }
  const std::string normalized_format =
      StandardReportRendererRegistry::NormalizeFormat(format_name);
  if (normalized_format.empty()) {
    return false;
  }

  const auto query_result = QueryService::QueryMonth(*report_data_gateway_, month_str);
  if (!query_result.data_found) {
    return false;
  }

  const auto standard_report = ReportRenderService::BuildStandardReport(query_result);
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    if (!write_standard_json("months/" + std::to_string(year), month_str,
                             ReportRenderService::Render(standard_report, "json"))) {
      return false;
    }
  }
  const std::string report = ReportRenderService::Render(standard_report, normalized_format);
  if (!suppress_output) {
    std::cout << report;
  }
  const auto folder_it = format_folder_names_.find(normalized_format);
  const std::string folder_name =
      folder_it == format_folder_names_.end() ? normalized_format : folder_it->second;
  return write_report(folder_name, "months/" + std::to_string(year), month_str,
                      extension_for_format(normalized_format), report);
}

auto ReportExportService::export_by_date(const std::string& date_str,
                                         const std::string& format_name) -> bool {
  if (date_str.length() == kYearLength) {
    int parsed_year = 0;
    if (parse_iso_year(date_str, parsed_year)) {
      return export_yearly_report(date_str, format_name, false);
    }
  }
  int parsed_year = 0;
  int parsed_month = 0;
  if (parse_iso_month(date_str, parsed_year, parsed_month)) {
    return export_monthly_report(date_str, format_name, false);
  }
  return false;
}

auto ReportExportService::export_by_date_range(const std::string& start_date,
                                               const std::string& end_date,
                                               const std::string& format_name)
    -> bool {
  int start_year = 0;
  int start_month = 0;
  int end_year = 0;
  int end_month = 0;
  if (!parse_iso_month(start_date, start_year, start_month) ||
      !parse_iso_month(end_date, end_year, end_month) ||
      month_key(start_year, start_month) > month_key(end_year, end_month)) {
    return false;
  }

  const auto all_months = report_data_gateway_->ListAvailableMonths();
  bool success = true;
  for (const auto& month : all_months) {
    int current_year = 0;
    int current_month = 0;
    if (!parse_iso_month(month, current_year, current_month)) {
      continue;
    }
    const int current_key = month_key(current_year, current_month);
    if (current_key >= month_key(start_year, start_month) &&
        current_key <= month_key(end_year, end_month)) {
      success = export_monthly_report(month, format_name, true) && success;
    }
  }
  return success;
}

auto ReportExportService::export_all_monthly_reports(const std::string& format_name)
    -> bool {
  const auto all_months = report_data_gateway_->ListAvailableMonths();
  bool success = true;
  for (const auto& month : all_months) {
    success = export_monthly_report(month, format_name, true) && success;
  }
  return success;
}

auto ReportExportService::export_all_yearly_reports(const std::string& format_name)
    -> bool {
  const auto all_months = report_data_gateway_->ListAvailableMonths();
  std::set<std::string> years;
  for (const auto& month : all_months) {
    if (month.size() >= 4U) {
      years.insert(month.substr(0, 4));
    }
  }
  bool success = true;
  for (const auto& year : years) {
    success = export_yearly_report(year, format_name, true) && success;
  }
  return success;
}

auto ReportExportService::export_all_reports(const std::string& format_name) -> bool {
  return export_all_monthly_reports(format_name) &&
         export_all_yearly_reports(format_name);
}
