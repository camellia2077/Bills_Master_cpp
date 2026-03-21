#include "presentation/features/report/report_handler.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include "bills_io/adapters/reports/report_export_service.hpp"
#include "bills_io/io_factory.hpp"
#include "common/Result.hpp"
#include "common/common_utils.hpp"
#include "query/query_service.hpp"
#include "reporting/report_render_service.hpp"

namespace terminal = common::terminal;

namespace bills::cli {
namespace {

auto CreateQueryGateway(const RuntimeContext& context)
    -> std::pair<std::unique_ptr<SqliteReportDbSession>,
                 std::unique_ptr<ReportDataGateway>> {
  auto db_session =
      bills::io::CreateReportDbSession(context.default_db_path.string());
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  return {std::move(db_session), std::move(report_data_gateway)};
}

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

        auto [db_session, report_data_gateway] = CreateQueryGateway(context_);
        QueryExecutionResult query_result;
        if (request.action == ReportAction::kShowYear) {
          query_result =
              QueryService::QueryYear(*report_data_gateway, request.primary_value);
        } else {
          query_result =
              QueryService::QueryMonth(*report_data_gateway, request.primary_value);
        }
        if (!query_result.data_found) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << "No report data found for '" << request.primary_value
                    << "'." << '\n';
          return false;
        }

        const auto standard_report =
            ReportRenderService::BuildStandardReport(query_result);
        PrintRenderedReport(ReportRenderService::Render(standard_report, *format));
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

        bool success = true;
        auto db_session =
            bills::io::CreateReportDbSession(context_.default_db_path.string());
        auto report_data_gateway =
            bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
        ReportExportService export_service(std::move(report_data_gateway),
                                           context_.export_dir.string(),
                                           context_.format_folder_names);
        for (const auto& format : *formats) {
          bool current_success = false;
          if (request.action == ReportAction::kExportYear) {
            current_success =
                export_service.export_yearly_report(request.primary_value, format);
          } else if (request.action == ReportAction::kExportMonth) {
            current_success =
                export_service.export_monthly_report(request.primary_value, format);
          } else if (request.action == ReportAction::kExportRange) {
            current_success = export_service.export_by_date_range(
                request.primary_value, request.secondary_value, format);
          } else if (request.action == ReportAction::kExportAllMonths) {
            current_success = export_service.export_all_monthly_reports(format);
          } else if (request.action == ReportAction::kExportAllYears) {
            current_success = export_service.export_all_yearly_reports(format);
          } else {
            current_success = export_service.export_all_reports(format);
          }

          if (!current_success) {
            std::cerr << terminal::kRed << "Error: " << terminal::kReset
                      << "Report export failed for format '" << format << "'."
                      << '\n';
          }
          success = current_success && success;
        }
        return success;
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
