// reports/core/ReportExportService.hpp
#ifndef REPORT_EXPORT_SERVICE_HPP
#define REPORT_EXPORT_SERVICE_HPP

#include <map>
#include <memory>
#include <string>

class ReportDataGateway;
class MonthReportFormatterProvider;
class YearlyReportFormatterProvider;
class ReportExporter;

class ReportExportService {
 public:
  // Constructor with fully injected adapters/providers
  explicit ReportExportService(
      std::unique_ptr<ReportDataGateway> report_data_gateway,
      std::unique_ptr<MonthReportFormatterProvider> month_formatter_provider,
      std::unique_ptr<YearlyReportFormatterProvider> year_formatter_provider,
      const std::string& export_base_dir = "exported_files",
      const std::map<std::string, std::string>& format_folder_names = {});

  ~ReportExportService();

  ReportExportService(const ReportExportService&) = delete;
  ReportExportService& operator=(const ReportExportService&) = delete;

  // Report Export APIs
  bool export_yearly_report(const std::string& year_str,
                            const std::string& format_name,
                            bool suppress_output = false,
                            const std::string& export_pipeline = "model-first");
  bool export_monthly_report(const std::string& month_str,
                             const std::string& format_name,
                             bool suppress_output = false,
                             const std::string& export_pipeline = "model-first");
  bool export_by_date(const std::string& date_str,
                      const std::string& format_name,
                      const std::string& export_pipeline = "model-first");
  bool export_by_date_range(const std::string& start_date,
                            const std::string& end_date,
                            const std::string& format_name,
                            const std::string& export_pipeline = "model-first");

  // Batch Export APIs
  bool export_all_reports(const std::string& format_name,
                          const std::string& export_pipeline = "model-first");
  bool export_all_monthly_reports(
      const std::string& format_name,
      const std::string& export_pipeline = "model-first");
  bool export_all_yearly_reports(const std::string& format_name,
                                 const std::string& export_pipeline = "model-first");

 private:
  std::unique_ptr<ReportDataGateway> m_report_data_gateway;
  std::unique_ptr<MonthReportFormatterProvider> m_month_formatter_provider;
  std::unique_ptr<YearlyReportFormatterProvider> m_year_formatter_provider;
  std::unique_ptr<ReportExporter> m_report_exporter;
};

#endif  // REPORT_EXPORT_SERVICE_HPP
