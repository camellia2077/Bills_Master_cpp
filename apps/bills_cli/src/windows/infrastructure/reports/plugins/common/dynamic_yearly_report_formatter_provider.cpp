#include "windows/infrastructure/reports/plugins/common/dynamic_yearly_report_formatter_provider.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "reports/plugins/year_formatters/year_md/year_md_format.hpp"

namespace {
constexpr const char* kYearFormatterSuffix = "_year_formatter";

auto normalize_format(std::string format_name) -> std::string {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return format_name;
}
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
  const std::string normalized = normalize_format(std::string(format_name));
  if (normalized == "md" || normalized == "markdown") {
    return std::make_unique<YearMdFormat>();
  }
  return plugin_loader_.createFormatter(normalized);
}
