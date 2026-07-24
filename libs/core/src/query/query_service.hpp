#ifndef QUERY_QUERY_SERVICE_HPP_
#define QUERY_QUERY_SERVICE_HPP_

#include <string>

#include "ports/contracts/reports/range/range_report_data.hpp"
#include "ports/report_data_gateway.hpp"

struct QueryExecutionResult {
  std::string period_start;
  std::string period_end;
  bool data_found = false;
  RangeReportData range_data;
};

class QueryService {
 public:
  [[nodiscard]] static auto QueryRange(ReportDataGateway& gateway,
                                       std::string_view start_iso_month,
                                       std::string_view end_iso_month)
      -> QueryExecutionResult;
};

#endif  // QUERY_QUERY_SERVICE_HPP_
