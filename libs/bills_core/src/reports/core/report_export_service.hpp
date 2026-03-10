// reports/core/report_export_service.hpp
#ifndef REPORTS_CORE_REPORT_EXPORT_SERVICE_H_
#define REPORTS_CORE_REPORT_EXPORT_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

class ReportDataGateway;
class ReportExporter;

class ReportExportService {
 public:
  explicit ReportExportService(
      std::unique_ptr<ReportDataGateway> report_data_gateway,
      const std::string& export_base_dir = "exports",
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
  [[nodiscard]] static auto ListAvailableFormats() -> std::vector<std::string>;

 private:
  std::unique_ptr<ReportDataGateway> m_report_data_gateway;
  std::unique_ptr<ReportExporter> m_report_exporter;
};

#endif  // REPORTS_CORE_REPORT_EXPORT_SERVICE_H_
