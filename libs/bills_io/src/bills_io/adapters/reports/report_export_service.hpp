#ifndef BILLS_IO_ADAPTERS_REPORTS_REPORT_EXPORT_SERVICE_HPP_
#define BILLS_IO_ADAPTERS_REPORTS_REPORT_EXPORT_SERVICE_HPP_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class ReportDataGateway;

struct ReportExportYear {
  std::string iso_year;
  int year = 0;
};

struct ReportExportMonth {
  std::string iso_month;
  int year = 0;
  int month = 0;
};

struct ReportExportRange {
  ReportExportMonth start;
  ReportExportMonth end;
};

[[nodiscard]] auto TryBuildReportExportYear(std::string_view raw)
    -> std::optional<ReportExportYear>;

[[nodiscard]] auto TryBuildReportExportMonth(std::string_view raw)
    -> std::optional<ReportExportMonth>;

[[nodiscard]] inline auto ReportExportMonthKey(const ReportExportMonth& month)
    -> int {
  return month.year * 100 + month.month;
}

[[nodiscard]] inline auto IsReportExportRangeInOrder(
    const ReportExportRange& range) -> bool {
  return ReportExportMonthKey(range.start) <= ReportExportMonthKey(range.end);
}

class ReportExportService {
 public:
  explicit ReportExportService(
      std::unique_ptr<ReportDataGateway> report_data_gateway,
      const std::string& export_base_dir = "exports",
      const std::map<std::string, std::string>& format_folder_names = {});

  bool export_yearly_report(const ReportExportYear& year,
                            const std::string& format_name);
  bool export_monthly_report(const ReportExportMonth& month,
                             const std::string& format_name);
  bool export_monthly_range(const ReportExportRange& range,
                            const std::string& format_name);
  bool export_all_reports(const std::string& format_name);
  bool export_all_monthly_reports(const std::string& format_name);
  bool export_all_yearly_reports(const std::string& format_name);
  [[nodiscard]] static auto ListAvailableFormats() -> std::vector<std::string>;

 private:
  struct NormalizedAvailableMonths {
    std::vector<ReportExportMonth> months;
    bool had_invalid_entries = false;
  };

  [[nodiscard]] auto list_normalized_available_months() const
      -> NormalizedAvailableMonths;
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
