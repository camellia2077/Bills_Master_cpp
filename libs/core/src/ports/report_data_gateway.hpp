// ports/report_data_gateway.hpp
#ifndef PORTS_REPORT_DATA_GATEWAY_H_
#define PORTS_REPORT_DATA_GATEWAY_H_

#include <string>
#include <string_view>
#include <vector>

#include "ports/contracts/reports/range/range_report_data.hpp"

class ReportDataGateway {
 public:
  virtual ~ReportDataGateway() = default;

  [[nodiscard]] virtual auto ReadRangeData(std::string_view start_iso_month,
                                           std::string_view end_iso_month)
      -> RangeReportData = 0;
  [[nodiscard]] virtual auto ListAvailableMonths()
      -> std::vector<std::string> = 0;
};

#endif  // PORTS_REPORT_DATA_GATEWAY_H_
