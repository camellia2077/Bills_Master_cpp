// ports/yearly_report_formatter_provider.hpp
#ifndef PORTS_YEARLY_REPORT_FORMATTER_PROVIDER_H_
#define PORTS_YEARLY_REPORT_FORMATTER_PROVIDER_H_

#include <memory>
#include <string_view>

#include "ports/contracts/reports/formatters/i_yearly_report_formatter.hpp"

class YearlyReportFormatterProvider {
 public:
  virtual ~YearlyReportFormatterProvider() = default;

  [[nodiscard]] virtual auto CreateFormatter(std::string_view format_name)
      -> std::unique_ptr<IYearlyReportFormatter> = 0;
};

#endif  // PORTS_YEARLY_REPORT_FORMATTER_PROVIDER_H_
