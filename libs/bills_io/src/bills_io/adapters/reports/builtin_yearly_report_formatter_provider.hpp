// bills_io/adapters/reports/builtin_yearly_report_formatter_provider.hpp
#ifndef BILLS_IO_ADAPTERS_REPORTS_BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_H_
#define BILLS_IO_ADAPTERS_REPORTS_BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_H_

#include "ports/yearly_report_formatter_provider.hpp"

class BuiltinYearlyReportFormatterProvider final
    : public YearlyReportFormatterProvider {
 public:
  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IYearlyReportFormatter> override;
};

#endif  // BILLS_IO_ADAPTERS_REPORTS_BUILTIN_YEARLY_REPORT_FORMATTER_PROVIDER_H_
