#include "windows/infrastructure/reports/plugins/common/dynamic_yearly_report_formatter_provider.hpp"

#include <string>

namespace {
constexpr const char* kYearFormatterSuffix = "_year_formatter";
}  // namespace

DynamicYearlyReportFormatterProvider::DynamicYearlyReportFormatterProvider(
    const std::string& plugin_directory_path)
    : plugin_loader_(kYearFormatterSuffix) {
  plugin_loader_.loadPluginsFromDirectory(plugin_directory_path);
}

DynamicYearlyReportFormatterProvider::DynamicYearlyReportFormatterProvider(
    const std::vector<std::string>& plugin_file_paths)
    : plugin_loader_(kYearFormatterSuffix) {
  plugin_loader_.loadPluginsFromFiles(plugin_file_paths);
}

auto DynamicYearlyReportFormatterProvider::CreateFormatter(
    std::string_view format_name) -> std::unique_ptr<IYearlyReportFormatter> {
  return plugin_loader_.createFormatter(std::string(format_name));
}

