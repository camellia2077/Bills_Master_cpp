#ifndef DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP
#define DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP

#include <functional>
#include <string>
#include <vector>

#include "bills_io/adapters/plugins/common/plugin_loader.hpp"
#include "ports/month_report_formatter_provider.hpp"

class DynamicMonthReportFormatterProvider final
    : public MonthReportFormatterProvider {
 public:
  using FallbackFactory =
      std::function<std::unique_ptr<IMonthReportFormatter>(std::string_view)>;

  explicit DynamicMonthReportFormatterProvider(
      const std::string& plugin_directory_path);
  explicit DynamicMonthReportFormatterProvider(
      const std::vector<std::string>& plugin_file_paths);
  DynamicMonthReportFormatterProvider(
      const std::vector<std::string>& plugin_file_paths,
      FallbackFactory fallback_factory);

  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> override;

 private:
  PluginLoader<IMonthReportFormatter> plugin_loader_;
  FallbackFactory fallback_factory_;
};

#endif  // DYNAMIC_MONTH_REPORT_FORMATTER_PROVIDER_HPP
