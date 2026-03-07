// bills_io/adapters/reports/builtin_yearly_report_formatter_provider.cpp
#include "bills_io/adapters/reports/builtin_yearly_report_formatter_provider.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#if BILLS_FMT_MD_ENABLED
#include "reports/formatters/year/md/year_md_format.hpp"
#endif
#if BILLS_FMT_RST_ENABLED
#include "reports/formatters/year/rst/year_rst_format.hpp"
#endif
#if BILLS_FMT_TEX_ENABLED
#include "reports/formatters/year/tex/year_tex_format.hpp"
#endif

namespace {
auto normalize_format(std::string format_name) -> std::string {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return format_name;
}
}  // namespace

auto BuiltinYearlyReportFormatterProvider::CreateFormatter(
    std::string_view format_name) -> std::unique_ptr<IYearlyReportFormatter> {
  const std::string normalized = normalize_format(std::string(format_name));

#if BILLS_FMT_MD_ENABLED
  if (normalized == "md" || normalized == "markdown") {
    return std::make_unique<YearMdFormat>();
  }
#endif
#if BILLS_FMT_RST_ENABLED
  if (normalized == "rst") {
    return std::make_unique<YearRstFormat>();
  }
#endif
#if BILLS_FMT_TEX_ENABLED
  if (normalized == "tex" || normalized == "latex") {
    return std::make_unique<YearTexFormat>();
  }
#endif

  return nullptr;
}
