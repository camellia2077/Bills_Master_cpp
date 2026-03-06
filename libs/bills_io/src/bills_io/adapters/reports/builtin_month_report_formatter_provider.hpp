#ifndef BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_HPP
#define BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_HPP

#include "ports/month_report_formatter_provider.hpp"

class BuiltinMonthReportFormatterProvider final
    : public MonthReportFormatterProvider {
 public:
  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> override;
};

#endif  // BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_HPP
