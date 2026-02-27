// controllers/export/ExportController.cpp

#include "ExportController.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "common/common_utils.hpp"  // 确保包含颜色定义
#include "platform/windows/infrastructure/adapters/db/SqliteReportDataGateway.hpp"
#include "platform/windows/infrastructure/adapters/db/SqliteReportDbSession.hpp"
#include "platform/windows/infrastructure/reports/plugins/common/DynamicMonthReportFormatterProvider.hpp"
#include "platform/windows/infrastructure/reports/plugins/common/DynamicYearlyReportFormatterProvider.hpp"
#include "reports/core/ReportExportService.hpp"

ExportController::ExportController(
    std::string db_path, const std::vector<std::string>& plugin_files,
    std::string export_base_dir,
    const std::map<std::string, std::string>& format_folder_names)
    : m_db_path(std::move(db_path)),
      m_plugin_files(plugin_files),
      m_export_base_dir(std::move(export_base_dir)),
      m_format_folder_names(format_folder_names) {}

// handle_export 函数的实现代码与 AppController.cpp
// 中的完全相同，直接复制过来即可。
auto ExportController::handle_export(const std::string& type,
                                     const std::vector<std::string>& values,
                                     const std::string& format_str,
                                     const std::string& export_pipeline)
    -> bool {
  bool success = false;
  try {
    auto db_session = std::make_unique<SqliteReportDbSession>(m_db_path);
    auto report_data_gateway = std::make_unique<SqliteReportDataGateway>(
        db_session->GetConnectionHandle());
    auto month_formatter_provider =
        std::make_unique<DynamicMonthReportFormatterProvider>(m_plugin_files);
    auto year_formatter_provider =
        std::make_unique<DynamicYearlyReportFormatterProvider>(m_plugin_files);
    ReportExportService report_export_service(
        std::move(report_data_gateway), std::move(month_formatter_provider),
        std::move(year_formatter_provider), m_export_base_dir,
        m_format_folder_names);

    if (type == "all") {
      success =
          report_export_service.export_all_reports(format_str, export_pipeline);
    } else if (type == "all_months") {
      success = report_export_service.export_all_monthly_reports(
          format_str, export_pipeline);
    } else if (type == "all_years") {
      success = report_export_service.export_all_yearly_reports(
          format_str, export_pipeline);
    } else if (type == "date") {
      if (values.empty()) {
        throw std::runtime_error(
            "At least one date string must be provided for 'date' export.");
      }
      if (values.size() == 1) {  // 单个日期
        success = report_export_service.export_by_date(values[0], format_str,
                                                       export_pipeline);
      } else if (values.size() == 2) {  // 日期区间
        success = report_export_service.export_by_date_range(
            values[0], values[1], format_str, export_pipeline);
      } else {
        throw std::runtime_error(
            "For 'date' export, please provide one (YYYY or YYYY-MM) or two "
            "(YYYY-MM YYYY-MM) date values.");
      }
    } else if (type == "year") {
      if (values.empty() || values[0].empty()) {
        throw std::runtime_error(
            "A year must be provided to export a yearly report.");
      }
      success = report_export_service.export_yearly_report(
          values[0], format_str, false, export_pipeline);
    } else if (type == "month") {
      if (values.empty() || values[0].empty()) {
        throw std::runtime_error(
            "A month (YYYY-MM) must be provided to export a monthly report.");
      }
      success = report_export_service.export_monthly_report(
          values[0], format_str, false, export_pipeline);
    } else {
      throw std::runtime_error("Unknown export type: " + type);
    }
  } catch (const std::exception& e) {
    std::cerr << RED_COLOR << "Export failed: " << RESET_COLOR << e.what()
              << std::endl;
    return false;
  }
  return success;
}
