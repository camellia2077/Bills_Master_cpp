#ifndef MONTH_REPORT_FORMATTER_PROVIDER_HPP
#define MONTH_REPORT_FORMATTER_PROVIDER_HPP

#include <memory>
#include <string_view>

#include "ports/contracts/reports/formatters/i_month_report_formatter.hpp"

class MonthReportFormatterProvider {
 public:
  virtual ~MonthReportFormatterProvider() = default;

  [[nodiscard]] virtual auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IMonthReportFormatter> = 0;
};

#endif  // MONTH_REPORT_FORMATTER_PROVIDER_HPP
