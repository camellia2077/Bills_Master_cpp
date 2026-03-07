// bills_io/adapters/reports/builtin_month_report_formatter_provider.hpp
#ifndef BILLS_IO_ADAPTERS_REPORTS_BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_H_
#define BILLS_IO_ADAPTERS_REPORTS_BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_H_

#include "ports/month_report_formatter_provider.hpp"

class BuiltinMonthReportFormatterProvider final
    : public MonthReportFormatterProvider {
 public:
  [[nodiscard]] auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> override;
};

#endif  // BILLS_IO_ADAPTERS_REPORTS_BUILTIN_MONTH_REPORT_FORMATTER_PROVIDER_H_
