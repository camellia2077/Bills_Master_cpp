#ifndef BILLS_IO_ADAPTERS_REPORTS_REPORT_EXPORT_SERVICE_HPP_
#define BILLS_IO_ADAPTERS_REPORTS_REPORT_EXPORT_SERVICE_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

class ReportDataGateway;

class ReportExportService {
 public:
  explicit ReportExportService(
      std::unique_ptr<ReportDataGateway> report_data_gateway,
      const std::string& export_base_dir = "exports",
      const std::map<std::string, std::string>& format_folder_names = {});

  bool export_yearly_report(const std::string& year_str,
                            const std::string& format_name,
                            bool suppress_output = false);
  bool export_monthly_report(const std::string& month_str,
                             const std::string& format_name,
                             bool suppress_output = false);
  bool export_by_date(const std::string& date_str,
                      const std::string& format_name);
  bool export_by_date_range(const std::string& start_date,
                            const std::string& end_date,
                            const std::string& format_name);
  bool export_all_reports(const std::string& format_name);
  bool export_all_monthly_reports(const std::string& format_name);
  bool export_all_yearly_reports(const std::string& format_name);
  [[nodiscard]] static auto ListAvailableFormats() -> std::vector<std::string>;

 private:
  bool write_report(const std::string& folder_name, const std::string& group_name,
                    const std::string& stem, const std::string& extension,
                    const std::string& content) const;
  bool write_standard_json(const std::string& group_name, const std::string& stem,
                           const std::string& content) const;

  std::unique_ptr<ReportDataGateway> report_data_gateway_;
  std::string export_base_dir_;
  std::map<std::string, std::string> format_folder_names_;
};

#endif  // BILLS_IO_ADAPTERS_REPORTS_REPORT_EXPORT_SERVICE_HPP_
