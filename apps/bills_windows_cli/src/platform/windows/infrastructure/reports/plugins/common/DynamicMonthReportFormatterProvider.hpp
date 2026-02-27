#ifndef DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP
#define DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP

#include <string>
#include <vector>

#include "platform/windows/infrastructure/reports/plugins/common/PluginLoader.hpp"
#include "ports/month_report_formatter_provider.hpp"

class DynamicMonthReportFormatterProvider final
    : public MonthReportFormatterProvider {
 public:
  explicit DynamicMonthReportFormatterProvider(
      const std::string& plugin_directory_path);
  explicit DynamicMonthReportFormatterProvider(
      const std::vector<std::string>& plugin_file_paths);

  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> override;

 private:
  PluginLoader<IMonthReportFormatter> plugin_loader_;
};

#endif  // DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP
