#include "platform/windows/infrastructure/reports/plugins/common/DynamicMonthReportFormatterProvider.hpp"

#include <string>

namespace {
constexpr const char* kMonthFormatterSuffix = "_month_formatter";
}  // namespace

DynamicMonthReportFormatterProvider::DynamicMonthReportFormatterProvider(
    const std::string& plugin_directory_path)
    : plugin_loader_(kMonthFormatterSuffix) {
  plugin_loader_.loadPluginsFromDirectory(plugin_directory_path);
}

DynamicMonthReportFormatterProvider::DynamicMonthReportFormatterProvider(
    const std::vector<std::string>& plugin_file_paths)
    : plugin_loader_(kMonthFormatterSuffix) {
  plugin_loader_.loadPluginsFromFiles(plugin_file_paths);
}

auto DynamicMonthReportFormatterProvider::CreateFormatter(
    std::string_view format_name) -> std::unique_ptr<IMonthReportFormatter> {
  return plugin_loader_.createFormatter(std::string(format_name));
}
