// bills_io/adapters/db/month_query.cpp

#include "month_query.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace {
constexpr int kMinSupportedYear = 1900;
constexpr int kMaxSupportedYear = 9999;

struct ParsedIsoMonth {
  int year = 0;
  int month = 0;
};

auto IsAsciiDigits(std::string_view value) -> bool {
  return !value.empty() &&
         std::all_of(value.begin(), value.end(), [](unsigned char ch) -> bool {
           return std::isdigit(ch) != 0;
         });
}

auto ParseIsoMonth(std::string_view iso_month) -> ParsedIsoMonth {
  if (iso_month.size() != 7U || iso_month[4] != '-' ||
      !IsAsciiDigits(iso_month.substr(0, 4)) ||
      !IsAsciiDigits(iso_month.substr(5, 2))) {
    throw std::invalid_argument("Month queries must use YYYY-MM.");
  }

  ParsedIsoMonth parsed;
  parsed.year = std::stoi(std::string(iso_month.substr(0, 4)));
  parsed.month = std::stoi(std::string(iso_month.substr(5, 2)));
  if (parsed.year < kMinSupportedYear || parsed.year > kMaxSupportedYear ||
      parsed.month < 1 || parsed.month > 12) {
    throw std::invalid_argument("Month queries must use YYYY-MM.");
  }
  return parsed;
}
}  // namespace

MonthQuery::MonthQuery(sqlite3* db_connection) : m_db(db_connection) {}

auto MonthQuery::read_monthly_data(std::string_view iso_month)
    -> MonthlyReportData {
  const ParsedIsoMonth parsed = ParseIsoMonth(iso_month);
  MonthlyReportData data;
  data.year = parsed.year;
  data.month = parsed.month;

  const char* totals_sql =
      "SELECT total_income, total_expense, balance, remark "
      "FROM bills WHERE bill_date = ?;";
  sqlite3_stmt* totals_stmt = nullptr;

  if (sqlite3_prepare_v2(m_db, totals_sql, -1, &totals_stmt, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("准备查询总计数据的 SQL 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }
  sqlite3_bind_text(totals_stmt, 1, iso_month.data(),
                    static_cast<int>(iso_month.size()), SQLITE_TRANSIENT);

  if (sqlite3_step(totals_stmt) == SQLITE_ROW) {
    data.data_found = true;
    data.total_income = sqlite3_column_double(totals_stmt, 0);
    data.total_expense = sqlite3_column_double(totals_stmt, 1);
    data.balance = sqlite3_column_double(totals_stmt, 2);
    const unsigned char* remark_raw = sqlite3_column_text(totals_stmt, 3);
    data.remark = (remark_raw != nullptr)
                      ? reinterpret_cast<const char*>(remark_raw)
                      : "";
  }
  sqlite3_finalize(totals_stmt);

  if (!data.data_found) {
    return data;
  }

  const char* sql =
      "SELECT t.parent_category, t.sub_category, t.amount, t.description "
      "FROM transactions AS t "
      "JOIN bills AS b ON t.bill_id = b.id "
      "WHERE b.bill_date = ?;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("准备查询交易的 SQL 语句失败: " +
                             std::string(sqlite3_errmsg(m_db)));
  }

  sqlite3_bind_text(stmt, 1, iso_month.data(), static_cast<int>(iso_month.size()),
                    SQLITE_TRANSIENT);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string parent_cat =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string sub_cat =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    double amount = sqlite3_column_double(stmt, 2);
    const unsigned char* desc_raw = sqlite3_column_text(stmt, 3);

    Transaction transaction;
    transaction.parent_category = parent_cat;
    transaction.sub_category = sub_cat;
    transaction.amount = amount;
    transaction.description =
        (desc_raw != nullptr) ? reinterpret_cast<const char*>(desc_raw) : "";

    data.aggregated_data[parent_cat].parent_total += amount;
    data.aggregated_data[parent_cat].sub_categories[sub_cat].sub_total +=
        amount;
    data.aggregated_data[parent_cat]
        .sub_categories[sub_cat]
        .transactions.push_back(transaction);
  }
  sqlite3_finalize(stmt);

  return data;
}
