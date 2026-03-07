// ports/month_report_formatter_provider.hpp
#ifndef PORTS_MONTH_REPORT_FORMATTER_PROVIDER_H_
#define PORTS_MONTH_REPORT_FORMATTER_PROVIDER_H_

#include <memory>
#include <string_view>

#include "ports/contracts/reports/formatters/i_month_report_formatter.hpp"

class MonthReportFormatterProvider {
 public:
  virtual ~MonthReportFormatterProvider() = default;

  [[nodiscard]] virtual auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> = 0;
};

#endif  // PORTS_MONTH_REPORT_FORMATTER_PROVIDER_H_
