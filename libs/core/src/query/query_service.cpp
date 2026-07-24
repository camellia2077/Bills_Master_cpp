#include "query/query_service.hpp"

auto QueryService::QueryRange(ReportDataGateway& gateway,
                              std::string_view start_iso_month,
                              std::string_view end_iso_month)
    -> QueryExecutionResult {
  QueryExecutionResult result;
  result.period_start = std::string(start_iso_month);
  result.period_end = std::string(end_iso_month);
  result.range_data = gateway.ReadRangeData(start_iso_month, end_iso_month);
  result.data_found = result.range_data.data_found;
  return result;
}
