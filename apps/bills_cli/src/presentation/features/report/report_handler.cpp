#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.features.report_handler;
import bill.cli.presentation.entry.runtime_context;
import bill.cli.presentation.parsing.cli_request;
import bill.cli.deps.io_host_flow_support;
import bill.cli.deps.common_utils;
#else
#include <presentation/features/report/report_handler.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>

#include <iostream>

namespace terminal = common::terminal;

namespace bills::cli {
namespace {

auto PrintRenderedReport(std::string content) -> void {
  std::cout << content;
  if (!content.empty() && content.back() != '\n') {
    std::cout << '\n';
  }
}

}  // namespace

ReportHandler::ReportHandler(const RuntimeContext& context) : context_(context) {}

auto ReportHandler::Handle(const ReportRequest& request) const -> bool {
  try {
    switch (request.action) {
      case ReportAction::kShowYear:
      case ReportAction::kShowMonth: {
        const auto format = ResolveSingleReportFormat(context_, request.format);
        if (!format) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(format.error()) << '\n';
          return false;
        }

        Result<bills::io::HostQueryResult> query_result;
        if (request.action == ReportAction::kShowYear) {
          query_result = bills::io::QueryYearReport(
              context_.default_db_path, request.primary_value);
        } else {
          query_result = bills::io::QueryMonthReport(
              context_.default_db_path, request.primary_value);
        }
        if (!query_result) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(query_result.error()) << '\n';
          return false;
        }
        if (!query_result->execution.data_found) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << "No report data found for '" << request.primary_value
                    << "'." << '\n';
          return false;
        }

        const auto rendered = bills::io::RenderQueryReport(*query_result, *format);
        if (!rendered) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(rendered.error()) << '\n';
          return false;
        }
        PrintRenderedReport(*rendered);
        return true;
      }

      case ReportAction::kExportYear:
      case ReportAction::kExportMonth:
      case ReportAction::kExportRange:
      case ReportAction::kExportAllMonths:
      case ReportAction::kExportAllYears:
      case ReportAction::kExportAll: {
        const auto formats = ResolveExportFormats(context_, request.format);
        if (!formats) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(formats.error()) << '\n';
          return false;
        }

        bills::io::HostReportExportRequest export_request;
        export_request.formats = *formats;
        export_request.db_path = context_.default_db_path;
        export_request.export_dir = context_.export_dir;
        export_request.format_folder_names = context_.format_folder_names;
        export_request.primary_value = request.primary_value;
        export_request.secondary_value = request.secondary_value;
        switch (request.action) {
          case ReportAction::kExportYear:
            export_request.scope = bills::io::HostReportExportScope::kYear;
            break;
          case ReportAction::kExportMonth:
            export_request.scope = bills::io::HostReportExportScope::kMonth;
            break;
          case ReportAction::kExportRange:
            export_request.scope = bills::io::HostReportExportScope::kRange;
            break;
          case ReportAction::kExportAllMonths:
            export_request.scope = bills::io::HostReportExportScope::kAllMonths;
            break;
          case ReportAction::kExportAllYears:
            export_request.scope = bills::io::HostReportExportScope::kAllYears;
            break;
          case ReportAction::kExportAll:
            export_request.scope = bills::io::HostReportExportScope::kAll;
            break;
          case ReportAction::kShowYear:
          case ReportAction::kShowMonth:
            break;
        }
        const auto export_result = bills::io::ExportReports(export_request);
        if (!export_result) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(export_result.error()) << '\n';
          return false;
        }
        for (const auto& failed_format : export_result->failed_formats) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << "Report export failed for format '" << failed_format
                    << "'." << '\n';
        }
        return export_result->ok;
      }
    }
  } catch (const std::exception& error) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << error.what() << '\n';
    return false;
  }

  return false;
}

}  // namespace bills::cli
