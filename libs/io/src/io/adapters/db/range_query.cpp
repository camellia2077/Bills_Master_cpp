#include "io/adapters/db/range_query.hpp"

#include <stdexcept>
#include <string>

#include "common/iso_period.hpp"
#include "io/adapters/db/month_query.hpp"

namespace {

auto IncrementMonth(bills::core::common::iso_period::IsoYearMonth value)
    -> bills::core::common::iso_period::IsoYearMonth {
  ++value.month;
  if (value.month > 12) {
    value.month = 1;
    ++value.year;
  }
  return value;
}

auto MonthKey(const bills::core::common::iso_period::IsoYearMonth& value) -> int {
  return value.year * 100 + value.month;
}

}  // namespace

RangeQuery::RangeQuery(sqlite3* db_connection) : db_connection_(db_connection) {}

auto RangeQuery::read_range_data(std::string_view start_iso_month,
                                 std::string_view end_iso_month)
    -> RangeReportData {
  const auto start = bills::core::common::iso_period::parse_year_month(start_iso_month);
  const auto end = bills::core::common::iso_period::parse_year_month(end_iso_month);
  if (!start.has_value() || !end.has_value()) {
    throw std::invalid_argument("Range queries must use YYYY-MM.");
  }
  if (MonthKey(*start) > MonthKey(*end)) {
    throw std::invalid_argument("Range queries require start <= end.");
  }

  RangeReportData result;
  result.period_start =
      bills::core::common::iso_period::format_year_month(start->year, start->month);
  result.period_end =
      bills::core::common::iso_period::format_year_month(end->year, end->month);

  MonthQuery month_query(db_connection_);
  // Range queries are materialized by reading each covered month in order and
  // aggregating the found monthly reports. Months without data are skipped
  // instead of being emitted as synthetic empty entries.
  for (auto current = *start; MonthKey(current) <= MonthKey(*end);
       current = IncrementMonth(current)) {
    const std::string iso_month =
        bills::core::common::iso_period::format_year_month(current.year,
                                                           current.month);
    auto monthly = month_query.read_monthly_data(iso_month);
    if (!monthly.data_found) {
      continue;
    }
    result.data_found = true;
    result.total_income += monthly.total_income;
    result.total_expense += monthly.total_expense;
    result.balance += monthly.balance;
    result.months.push_back(std::move(monthly));
  }

  return result;
}
