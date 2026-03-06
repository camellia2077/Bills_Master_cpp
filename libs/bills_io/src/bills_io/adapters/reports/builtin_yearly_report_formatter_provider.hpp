#ifndef BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_HPP
#define BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_HPP

#include "ports/yearly_report_formatter_provider.hpp"

class BuiltinYearlyReportFormatterProvider final
    : public YearlyReportFormatterProvider {
 public:
  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IYearlyReportFormatter> override;
};

#endif  // BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_HPP
