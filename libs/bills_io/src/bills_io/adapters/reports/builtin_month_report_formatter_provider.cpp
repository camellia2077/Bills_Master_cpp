// bills_io/adapters/reports/builtin_month_report_formatter_provider.cpp
#include "bills_io/adapters/reports/builtin_month_report_formatter_provider.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#if BILLS_FMT_MD_ENABLED
#include "reports/formatters/month/md/month_md_format.hpp"
#endif
#if BILLS_FMT_RST_ENABLED
#include "reports/formatters/month/rst/month_rst_format.hpp"
#endif
#if BILLS_FMT_TEX_ENABLED
#include "reports/formatters/month/tex/month_tex_format.hpp"
#endif

namespace {
auto NormalizeFormat(std::string format_name) -> std::string {
  std::ranges::transform(format_name, format_name.begin(),
                         [](unsigned char character) -> char {
                           return static_cast<char>(std::tolower(character));
                         });
  return format_name;
}
}  // namespace

auto BuiltinMonthReportFormatterProvider::CreateFormatter(
    std::string_view format_name) -> std::unique_ptr<IMonthReportFormatter> {
  const std::string kNormalized = NormalizeFormat(std::string(format_name));

#if BILLS_FMT_MD_ENABLED
  if (kNormalized == "md" || kNormalized == "markdown") {
    return std::make_unique<MonthMdFormat>();
  }
#endif
#if BILLS_FMT_RST_ENABLED
  if (kNormalized == "rst") {
    return std::make_unique<MonthRstFormat>();
  }
#endif
#if BILLS_FMT_TEX_ENABLED
  if (kNormalized == "tex" || kNormalized == "latex") {
    return std::make_unique<MonthTexFormat>();
  }
#endif

  return nullptr;
}
