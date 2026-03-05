#ifndef DYNAMIC_YEARLY_REPORT_FORMATTER_PROVIDER_HPP
#define DYNAMIC_YEARLY_REPORT_FORMATTER_PROVIDER_HPP

#include <string>
#include <vector>

#include "windows/infrastructure/reports/plugins/common/plugin_loader.hpp"
#include "ports/yearly_report_formatter_provider.hpp"

class DynamicYearlyReportFormatterProvider final
    : public YearlyReportFormatterProvider {
 public:
  explicit DynamicYearlyReportFormatterProvider(
      const std::string& plugin_directory_path);
  explicit DynamicYearlyReportFormatterProvider(
      const std::vector<std::string>& plugin_file_paths);

  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IYearlyReportFormatter> override;

 private:
  PluginLoader<IYearlyReportFormatter> plugin_loader_;
};

#endif  // DYNAMIC_YEARLY_REPORT_FORMATTER_PROVIDER_HPP

