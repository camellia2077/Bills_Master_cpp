// io/adapters/db/year_query.cpp

#include "year_query.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace {
constexpr int kMinSupportedYear = 1900;
constexpr int kMaxSupportedYear = 9999;

auto IsAsciiDigits(std::string_view value) -> bool {
  return !value.empty() &&
         std::all_of(value.begin(), value.end(), [](unsigned char ch) -> bool {
           return std::isdigit(ch) != 0;
         });
}

auto ParseIsoYear(std::string_view iso_year) -> int {
  if (iso_year.size() != 4U || !IsAsciiDigits(iso_year)) {
    throw std::invalid_argument("Year queries must use YYYY.");
  }

  const int year = std::stoi(std::string(iso_year));
  if (year < kMinSupportedYear || year > kMaxSupportedYear) {
    throw std::invalid_argument("Year queries must use YYYY.");
  }
  return year;
}
}  // namespace

YearQuery::YearQuery(sqlite3* db_connection) : m_db(db_connection) {}

auto YearQuery::read_yearly_data(std::string_view iso_year) -> YearlyReportData {
  YearlyReportData data;
  data.year = ParseIsoYear(iso_year);

  const char* sql =
      "SELECT "
      "  month, "
      "  SUM(total_income), "
      "  SUM(total_expense) "
      "FROM bills "
      "WHERE substr(bill_date, 1, 4) = ? "
      "GROUP BY month "
      "ORDER BY month;";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("准备年度查询的 SQL 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }

  sqlite3_bind_text(stmt, 1, iso_year.data(),
                    static_cast<int>(iso_year.size()), SQLITE_TRANSIENT);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    data.data_found = true;
    int month = sqlite3_column_int(stmt, 0);
    double month_income = sqlite3_column_double(stmt, 1);
    double month_expense = sqlite3_column_double(stmt, 2);

    data.monthly_summary[month] = {.income = month_income,
                                   .expense = month_expense};

    data.total_income += month_income;
    data.total_expense += month_expense;
  }
  sqlite3_finalize(stmt);

  if (data.data_found) {
    data.balance = data.total_income + data.total_expense;
  }

  return data;
}
