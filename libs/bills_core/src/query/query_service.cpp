#include "query/query_service.hpp"

#include <string>

auto QueryService::QueryYear(ReportDataGateway& gateway, std::string_view iso_year)
    -> QueryExecutionResult {
  QueryExecutionResult result;
  result.query_type = "year";
  result.query_value = std::string(iso_year);
  result.yearly_data = gateway.ReadYearlyData(iso_year);
  result.year = result.yearly_data.year;
  result.data_found = result.yearly_data.data_found;
  return result;
}

auto QueryService::QueryMonth(ReportDataGateway& gateway, std::string_view iso_month)
    -> QueryExecutionResult {
  QueryExecutionResult result;
  result.query_type = "month";
  result.query_value = std::string(iso_month);
  result.monthly_data = gateway.ReadMonthlyData(iso_month);
  result.year = result.monthly_data.year;
  result.month = result.monthly_data.month;
  result.data_found = result.monthly_data.data_found;
  return result;
}
