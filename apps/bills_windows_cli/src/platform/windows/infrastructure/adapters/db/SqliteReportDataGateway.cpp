#include "platform/windows/infrastructure/adapters/db/SqliteReportDataGateway.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
constexpr int kFirstColumn = 0;
constexpr int kSecondColumn = 1;
constexpr int kThirdColumn = 2;
constexpr int kFourthColumn = 3;

constexpr int kYearBindIndex = 1;
constexpr int kMonthBindIndex = 2;
}  // namespace

SqliteReportDataGateway::SqliteReportDataGateway(sqlite3* db_connection)
    : db_connection_(db_connection) {
  if (db_connection_ == nullptr) {
    throw std::invalid_argument("Database connection must not be null.");
  }
}

auto SqliteReportDataGateway::ReadMonthlyData(int year, int month)
    -> MonthlyReportData {
  MonthlyReportData data;
  data.year = year;
  data.month = month;

  const char* totals_sql =
      "SELECT total_income, total_expense, balance, remark FROM bills WHERE "
      "year = ? AND month = ?;";
  sqlite3_stmt* totals_stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, totals_sql, -1, &totals_stmt,
                         nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare totals query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  sqlite3_bind_int(totals_stmt, kYearBindIndex, year);
  sqlite3_bind_int(totals_stmt, kMonthBindIndex, month);

  if (sqlite3_step(totals_stmt) == SQLITE_ROW) {
    data.data_found = true;
    data.total_income = sqlite3_column_double(totals_stmt, kFirstColumn);
    data.total_expense = sqlite3_column_double(totals_stmt, kSecondColumn);
    data.balance = sqlite3_column_double(totals_stmt, kThirdColumn);
    const unsigned char* remark_raw =
        sqlite3_column_text(totals_stmt, kFourthColumn);
    data.remark = (remark_raw != nullptr)
                      ? reinterpret_cast<const char*>(remark_raw)
                      : "";
  }
  sqlite3_finalize(totals_stmt);

  if (!data.data_found) {
    return data;
  }

  const char* transaction_sql =
      "SELECT t.parent_category, t.sub_category, t.amount, t.description "
      "FROM transactions AS t "
      "JOIN bills AS b ON t.bill_id = b.id "
      "WHERE b.year = ? AND b.month = ?;";
  sqlite3_stmt* transaction_stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, transaction_sql, -1, &transaction_stmt,
                         nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare transaction query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  sqlite3_bind_int(transaction_stmt, kYearBindIndex, year);
  sqlite3_bind_int(transaction_stmt, kMonthBindIndex, month);

  while (sqlite3_step(transaction_stmt) == SQLITE_ROW) {
    std::string parent_category = reinterpret_cast<const char*>(
        sqlite3_column_text(transaction_stmt, kFirstColumn));
    std::string sub_category = reinterpret_cast<const char*>(
        sqlite3_column_text(transaction_stmt, kSecondColumn));
    double amount = sqlite3_column_double(transaction_stmt, kThirdColumn);
    const unsigned char* description_raw =
        sqlite3_column_text(transaction_stmt, kFourthColumn);

    Transaction transaction;
    transaction.parent_category = parent_category;
    transaction.sub_category = sub_category;
    transaction.amount = amount;
    transaction.description =
        (description_raw != nullptr)
            ? reinterpret_cast<const char*>(description_raw)
            : "";

    data.aggregated_data[parent_category].parent_total += amount;
    data.aggregated_data[parent_category]
        .sub_categories[sub_category]
        .sub_total += amount;
    data.aggregated_data[parent_category]
        .sub_categories[sub_category]
        .transactions.push_back(transaction);
  }

  sqlite3_finalize(transaction_stmt);
  return data;
}

auto SqliteReportDataGateway::ReadYearlyData(int year) -> YearlyReportData {
  YearlyReportData data;
  data.year = year;

  const char* sql =
      "SELECT "
      "  month, "
      "  SUM(total_income), "
      "  SUM(total_expense) "
      "FROM bills "
      "WHERE year = ? "
      "GROUP BY month "
      "ORDER BY month;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, sql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare yearly query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  sqlite3_bind_int(stmt, kYearBindIndex, year);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    data.data_found = true;
    int month = sqlite3_column_int(stmt, kFirstColumn);
    double month_income = sqlite3_column_double(stmt, kSecondColumn);
    double month_expense = sqlite3_column_double(stmt, kThirdColumn);

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

auto SqliteReportDataGateway::ListAvailableMonths() -> std::vector<std::string> {
  std::vector<std::string> months;
  const char* sql =
      "SELECT DISTINCT year, month FROM bills ORDER BY year, month;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_connection_, sql, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare month list query: " +
                             std::string(sqlite3_errmsg(db_connection_)));
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int year = sqlite3_column_int(stmt, kFirstColumn);
    int month = sqlite3_column_int(stmt, kSecondColumn);

    std::stringstream month_stream;
    month_stream << year << "-" << std::setfill('0') << std::setw(2) << month;
    months.push_back(month_stream.str());
  }

  sqlite3_finalize(stmt);
  return months;
}
