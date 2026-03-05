#include "bills_io/adapters/plugins/common/dynamic_yearly_report_formatter_provider.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

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
    : plugin_loader_(kYearFormatterSuffix), fallback_factory_() {
  plugin_loader_.loadPluginsFromDirectory(plugin_directory_path);
}

DynamicYearlyReportFormatterProvider::DynamicYearlyReportFormatterProvider(
    const std::vector<std::string>& plugin_file_paths)
    : plugin_loader_(kYearFormatterSuffix), fallback_factory_() {
  plugin_loader_.loadPluginsFromFiles(plugin_file_paths);
}

DynamicYearlyReportFormatterProvider::DynamicYearlyReportFormatterProvider(
    const std::vector<std::string>& plugin_file_paths,
    FallbackFactory fallback_factory)
    : plugin_loader_(kYearFormatterSuffix),
      fallback_factory_(std::move(fallback_factory)) {
  plugin_loader_.loadPluginsFromFiles(plugin_file_paths);
}

auto DynamicYearlyReportFormatterProvider::CreateFormatter(
    std::string_view format_name) -> std::unique_ptr<IYearlyReportFormatter> {
  const std::string normalized = normalize_format(std::string(format_name));
  if (fallback_factory_) {
    if (auto formatter = fallback_factory_(normalized); formatter != nullptr) {
      return formatter;
    }
  }
  return plugin_loader_.createFormatter(normalized);
}
