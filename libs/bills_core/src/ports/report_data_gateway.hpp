// ports/report_data_gateway.hpp
#ifndef PORTS_REPORT_DATA_GATEWAY_H_
#define PORTS_REPORT_DATA_GATEWAY_H_

#include <string>
#include <string_view>
#include <vector>

#include "ports/contracts/reports/monthly/monthly_report_data.hpp"
#include "ports/contracts/reports/yearly/yearly_report_data.hpp"

class ReportDataGateway {
 public:
  virtual ~ReportDataGateway() = default;

  [[nodiscard]] virtual auto ReadMonthlyData(std::string_view iso_month)
      -> MonthlyReportData = 0;
  [[nodiscard]] virtual auto ReadYearlyData(std::string_view iso_year)
      -> YearlyReportData = 0;
  [[nodiscard]] virtual auto ListAvailableMonths()
      -> std::vector<std::string> = 0;
};

#endif  // PORTS_REPORT_DATA_GATEWAY_H_
