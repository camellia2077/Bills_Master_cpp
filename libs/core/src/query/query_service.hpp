#ifndef QUERY_QUERY_SERVICE_HPP_
#define QUERY_QUERY_SERVICE_HPP_

#include <optional>
#include <string>

#include "ports/report_data_gateway.hpp"
#include "ports/contracts/reports/monthly/monthly_report_data.hpp"
#include "ports/contracts/reports/yearly/yearly_report_data.hpp"

struct QueryExecutionResult {
  std::string query_type;
  std::string query_value;
  int year = 0;
  std::optional<int> month;
  bool data_found = false;
  MonthlyReportData monthly_data;
  YearlyReportData yearly_data;
};

class QueryService {
 public:
  [[nodiscard]] static auto QueryYear(ReportDataGateway& gateway,
                                      std::string_view iso_year)
      -> QueryExecutionResult;

  [[nodiscard]] static auto QueryMonth(ReportDataGateway& gateway,
                                       std::string_view iso_month)
      -> QueryExecutionResult;
};

#endif  // QUERY_QUERY_SERVICE_HPP_
