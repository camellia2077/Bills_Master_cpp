// reports/core/report_export_service.cpp
#include "report_export_service.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <utility>

#include "common/common_utils.hpp"

namespace terminal = common::terminal;
#include "standard_json_latex_renderer.hpp"
#include "standard_json_markdown_renderer.hpp"
#if BILLS_CORE_MODULES_ENABLED
import bill.core.common.process_stats;
import bill.core.ports.month_report_formatter_provider;
import bill.core.ports.report_data_gateway;
import bill.core.ports.yearly_report_formatter_provider;
import bill.core.reports.report_exporter;
import bill.core.reports.standard_report_assembler;
import bill.core.reports.standard_report_json_serializer;
using bills::core::modules::common_process_stats::ProcessStats;
using bills::core::modules::ports::MonthReportFormatterProvider;
using bills::core::modules::ports::ReportDataGateway;
using bills::core::modules::ports::YearlyReportFormatterProvider;
using bills::core::modules::reports::ReportExporter;
using bills::core::modules::reports::StandardReportAssembler;
using bills::core::modules::reports::MonthlyReportData;
using bills::core::modules::reports::StandardReport;
using bills::core::modules::reports::StandardReportJsonSerializer;
using bills::core::modules::reports::YearlyReportData;
#else
#include "common/process_stats.hpp"
#include "ports/month_report_formatter_provider.hpp"
#include "ports/report_data_gateway.hpp"
#include "ports/yearly_report_formatter_provider.hpp"
#include "report_exporter.hpp"
#include "reports/standard_json/standard_report_assembler.hpp"
#include "reports/standard_json/standard_report_json_serializer.hpp"
#endif

namespace {
constexpr std::size_t kYearLength = 4U;
const std::regex kIsoMonthRegex(R"(^(\d{4})-(0[1-9]|1[0-2])$)");

enum class StandardRenderMode {
  kNone,
  kJson,
  kMarkdown,
  kLatex,
};

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
  return true;
}

auto month_key(int year, int month) -> int { return year * 100 + month; }

auto is_markdown_format(std::string format_name) -> bool {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return format_name == "md" || format_name == "markdown";
}

auto is_json_format(std::string format_name) -> bool {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return format_name == "json";
}

auto is_latex_format(std::string format_name) -> bool {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return format_name == "tex" || format_name == "latex";
}

auto select_standard_render_mode(const std::string& pipeline,
                                 const std::string& format_name)
    -> StandardRenderMode {
  (void)pipeline;
  if (is_json_format(format_name)) {
    return StandardRenderMode::kJson;
  }
  if (is_markdown_format(format_name)) {
    return StandardRenderMode::kMarkdown;
  }
  if (is_latex_format(format_name)) {
    return StandardRenderMode::kLatex;
  }
  return StandardRenderMode::kNone;
}

auto render_standard_report(const StandardReport& standard_report,
                            const std::string& standard_report_json,
                            const StandardRenderMode mode) -> std::string {
  switch (mode) {
    case StandardRenderMode::kJson:
      return standard_report_json;
    case StandardRenderMode::kMarkdown:
      return StandardJsonMarkdownRenderer::render(standard_report);
    case StandardRenderMode::kLatex:
      return StandardJsonLatexRenderer::render(standard_report);
    case StandardRenderMode::kNone:
    default:
      return {};
  }
}

auto normalize_export_pipeline(std::string pipeline_name) -> std::string {
  std::ranges::transform(pipeline_name, pipeline_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  std::ranges::replace(pipeline_name, '_', '-');
  if (pipeline_name.empty()) {
    return "model-first";
  }
  if (pipeline_name == "jsonfirst") {
    return "json-first";
  }
  if (pipeline_name == "modelfirst") {
    return "model-first";
  }
  if (pipeline_name != "legacy" && pipeline_name != "model-first" &&
      pipeline_name != "json-first") {
    throw std::invalid_argument("Unknown export pipeline: " + pipeline_name);
  }
  return pipeline_name;
}
}  // namespace

// Constructor with fully injected adapters/providers
ReportExportService::ReportExportService(
    std::unique_ptr<ReportDataGateway> report_data_gateway,
    std::unique_ptr<MonthReportFormatterProvider> month_formatter_provider,
    std::unique_ptr<YearlyReportFormatterProvider> year_formatter_provider,
    const std::string& export_base_dir,
    const std::map<std::string, std::string>& format_folder_names)
    : m_report_data_gateway(std::move(report_data_gateway)),
      m_month_formatter_provider(std::move(month_formatter_provider)),
      m_year_formatter_provider(std::move(year_formatter_provider)) {
  if (m_report_data_gateway == nullptr) {
    throw std::invalid_argument("Report data gateway must not be null.");
  }
  if (m_month_formatter_provider == nullptr) {
    throw std::invalid_argument("Month formatter provider must not be null.");
  }
  if (m_year_formatter_provider == nullptr) {
    throw std::invalid_argument("Year formatter provider must not be null.");
  }

  m_report_exporter =
      std::make_unique<ReportExporter>(export_base_dir, format_folder_names);
}

ReportExportService::~ReportExportService() = default;

auto ReportExportService::export_yearly_report(const std::string& year_str,
                                               const std::string& format_name,
                                               bool suppress_output,
                                               const std::string& export_pipeline)
    -> bool {
  try {
    const std::string pipeline = normalize_export_pipeline(export_pipeline);
    const StandardRenderMode standard_render_mode =
        select_standard_render_mode(pipeline, format_name);
    int year = std::stoi(year_str);
    YearlyReportData yearly_data = m_report_data_gateway->ReadYearlyData(year);
    const auto standard_report =
        StandardReportAssembler::FromYearly(yearly_data);
    const std::string standard_report_json =
        StandardReportJsonSerializer::ToString(standard_report);
    m_report_exporter->export_yearly_standard_json(standard_report_json, year_str);

    std::string report;
    if (standard_render_mode != StandardRenderMode::kNone) {
      report = render_standard_report(standard_report, standard_report_json,
                                      standard_render_mode);
    } else {
      auto formatter = m_year_formatter_provider->CreateFormatter(format_name);
      if (!formatter) {
        throw std::runtime_error(
            "Yearly formatter for '" + format_name +
            "' is not available in the current build.");
      }
      report = formatter->format_report(yearly_data);
    }

    if (!suppress_output) {
      std::cout << report;
    }

    m_report_exporter->export_yearly(report, year_str, format_name);
    return true;
  } catch (const std::exception& e) {
    if (!suppress_output) {
      std::cerr << terminal::kRed << "Query Failed: " << terminal::kReset << e.what()
                << std::endl;
    }
    return false;
  }
}

auto ReportExportService::export_monthly_report(const std::string& month_str,
                                                const std::string& format_name,
                                                bool suppress_output,
                                                const std::string& export_pipeline)
    -> bool {
  try {
    const std::string pipeline = normalize_export_pipeline(export_pipeline);
    const StandardRenderMode standard_render_mode =
        select_standard_render_mode(pipeline, format_name);
    int year = 0;
    int month = 0;
    if (!parse_iso_month(month_str, year, month)) {
      throw std::invalid_argument("Invalid month format.");
    }
    MonthlyReportData monthly_data =
        m_report_data_gateway->ReadMonthlyData(year, month);
    const auto standard_report =
        StandardReportAssembler::FromMonthly(monthly_data);
    const std::string standard_report_json =
        StandardReportJsonSerializer::ToString(standard_report);
    m_report_exporter->export_monthly_standard_json(standard_report_json,
                                                    month_str);

    std::string report;
    if (standard_render_mode != StandardRenderMode::kNone) {
      report = render_standard_report(standard_report, standard_report_json,
                                      standard_render_mode);
    } else {
      auto formatter = m_month_formatter_provider->CreateFormatter(format_name);
      if (!formatter) {
        throw std::runtime_error(
            "Monthly formatter for '" + format_name +
            "' is not available in the current build.");
      }
      report = formatter->format_report(monthly_data);
    }

    if (!suppress_output) {
      std::cout << report;
    }

    m_report_exporter->export_monthly(report, month_str, format_name);
    return true;
  } catch (const std::exception& e) {
    if (!suppress_output) {
      std::cerr << terminal::kRed << "Query Failed: " << terminal::kReset << e.what()
                << std::endl;
    }
    return false;
  }
}

auto ReportExportService::export_by_date(const std::string& date_str,
                                         const std::string& format_name,
                                         const std::string& export_pipeline)
    -> bool {
  if (date_str.length() == kYearLength) {
    return export_yearly_report(date_str, format_name, false, export_pipeline);
  }
  int parsed_year = 0;
  int parsed_month = 0;
  if (parse_iso_month(date_str, parsed_year, parsed_month)) {
    return export_monthly_report(date_str, format_name, false,
                                 export_pipeline);
  }
  std::cerr << terminal::kRed << "Error:" << terminal::kReset
            << " Invalid date format for export: '" << date_str
            << "'. Please use YYYY or YYYY-MM.\n";
  return false;
}

auto ReportExportService::export_by_date_range(
    const std::string& start_date, const std::string& end_date,
    const std::string& format_name,
    const std::string& export_pipeline) -> bool {
  std::string pipeline;
  try {
    pipeline = normalize_export_pipeline(export_pipeline);
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "Error:" << terminal::kReset << " " << e.what()
              << "\n";
    return false;
  }

  int start_year = 0;
  int start_month = 0;
  int end_year = 0;
  int end_month = 0;
  if (!parse_iso_month(start_date, start_year, start_month) ||
      !parse_iso_month(end_date, end_year, end_month) ||
      month_key(start_year, start_month) > month_key(end_year, end_month)) {
    std::cerr << terminal::kRed << "Error:" << terminal::kReset
              << " Invalid date range. Use YYYY-MM format and ensure start_date "
                 "is not after end_date.\n";
    return false;
  }

  std::cout << "\n--- Exporting monthly reports from " << start_date << " to "
            << end_date << " (" << format_name << " format) ---\n";

  ProcessStats stats;
  try {
    std::vector<std::string> all_months =
        m_report_data_gateway->ListAvailableMonths();
    std::vector<std::string> months_to_export;
    for (const auto& month : all_months) {
      int current_year = 0;
      int current_month = 0;
      if (!parse_iso_month(month, current_year, current_month)) {
        continue;
      }
      const int current_key = month_key(current_year, current_month);
      if (current_key >= month_key(start_year, start_month) &&
          current_key <= month_key(end_year, end_month)) {
        months_to_export.push_back(month);
      }
    }

    if (months_to_export.empty()) {
      std::cout << terminal::kYellow << "Warning:" << terminal::kReset
                << " No data found within the specified date range.\n";
      return true;
    }

    for (const auto& month : months_to_export) {
      std::cout << "Exporting report for " << month << "...";
      if (export_monthly_report(month, format_name, true, pipeline)) {
        stats.success++;
        std::cout << terminal::kGreen << " OK\n" << terminal::kReset;
      } else {
        stats.failure++;
        std::cout << terminal::kRed << " FAILED\n" << terminal::kReset;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "An unexpected error occurred: " << terminal::kReset
              << e.what() << std::endl;
    stats.failure++;
  }

  stats.print_summary("Date Range Export");
  return stats.failure == 0;
}

auto ReportExportService::export_all_monthly_reports(
    const std::string& format_name,
    const std::string& export_pipeline) -> bool {
  std::string pipeline;
  try {
    pipeline = normalize_export_pipeline(export_pipeline);
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "Error:" << terminal::kReset << " " << e.what()
              << "\n";
    return false;
  }

  ProcessStats stats;
  std::cout << "\n--- Starting Monthly Report Export (" << format_name
            << " format) ---\n";
  try {
    std::vector<std::string> all_months =
        m_report_data_gateway->ListAvailableMonths();
    if (all_months.empty()) {
      std::cout << terminal::kYellow << "Warning: " << terminal::kReset
                << "No data found.\n";
      return true;
    }

    for (const auto& month : all_months) {
      std::cout << "Exporting report for " << month << "...";
      if (export_monthly_report(month, format_name, true, pipeline)) {
        stats.success++;
        std::cout << terminal::kGreen << " OK\n" << terminal::kReset;
      } else {
        stats.failure++;
        std::cout << terminal::kRed << " FAILED\n" << terminal::kReset;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "\nAn unexpected error occurred: " << terminal::kReset
              << e.what() << std::endl;
    return false;
  }
  stats.print_summary("Monthly Export");
  return stats.failure == 0;
}

auto ReportExportService::export_all_yearly_reports(
    const std::string& format_name,
    const std::string& export_pipeline) -> bool {
  std::string pipeline;
  try {
    pipeline = normalize_export_pipeline(export_pipeline);
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "Error:" << terminal::kReset << " " << e.what()
              << "\n";
    return false;
  }

  ProcessStats stats;
  std::cout << "\n--- Starting Yearly Report Export (" << format_name
            << " format) ---\n";
  try {
    std::vector<std::string> all_months =
        m_report_data_gateway->ListAvailableMonths();
    if (all_months.empty()) {
      std::cout << terminal::kYellow << "Warning: " << terminal::kReset
                << "No data found.\n";
      return true;
    }

    std::set<std::string> unique_years;
    for (const auto& month : all_months) {
      if (month.length() >= 4) {
        unique_years.insert(month.substr(0, 4));
      }
    }

    for (const auto& year : unique_years) {
      std::cout << "Exporting summary for " << year << "...";
      if (export_yearly_report(year, format_name, true, pipeline)) {
        stats.success++;
        std::cout << terminal::kGreen << " OK\n" << terminal::kReset;
      } else {
        stats.failure++;
        std::cout << terminal::kRed << " FAILED\n" << terminal::kReset;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "\nAn unexpected error occurred: " << terminal::kReset
              << e.what() << std::endl;
    return false;
  }
  stats.print_summary("Yearly Export");
  return stats.failure == 0;
}

auto ReportExportService::export_all_reports(const std::string& format_name,
                                             const std::string& export_pipeline)
    -> bool {
  std::string pipeline;
  try {
    pipeline = normalize_export_pipeline(export_pipeline);
  } catch (const std::exception& e) {
    std::cerr << terminal::kRed << "Error:" << terminal::kReset << " " << e.what()
              << "\n";
    return false;
  }

  std::cout << "\n--- Starting Full Report Export (" << format_name
            << " format) ---\n";
  bool monthly_ok = export_all_monthly_reports(format_name, pipeline);
  bool yearly_ok = export_all_yearly_reports(format_name, pipeline);
  bool overall_success = monthly_ok && yearly_ok;

  if (overall_success) {
    std::cout << "\n"
              << terminal::kGreen << "Success: " << terminal::kReset
              << "Full report export completed.\n";
  } else {
    std::cout << "\n"
              << terminal::kRed << "Failed: " << terminal::kReset
              << "Full report export completed with errors.\n";
  }
  return overall_success;
}

