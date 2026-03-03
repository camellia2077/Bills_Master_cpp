#ifndef YEARLY_REPORT_FORMATTER_PROVIDER_HPP
#define YEARLY_REPORT_FORMATTER_PROVIDER_HPP

#include <memory>
#include <string_view>

#include "ports/contracts/reports/formatters/i_yearly_report_formatter.hpp"

class YearlyReportFormatterProvider {
 public:
  virtual ~YearlyReportFormatterProvider() = default;

  [[nodiscard]] virtual auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IYearlyReportFormatter> = 0;
};

#endif  // YEARLY_REPORT_FORMATTER_PROVIDER_HPP
