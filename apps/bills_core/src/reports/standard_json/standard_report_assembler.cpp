#include "reports/standard_json/standard_report_assembler.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

auto now_utc_iso8601() -> std::string {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm {};
#if defined(_WIN32)
  gmtime_s(&utc_tm, &now_time);
#else
  gmtime_r(&now_time, &utc_tm);
#endif
  std::ostringstream output;
  output << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return output.str();
}

auto month_to_text(const int year, const int month) -> std::string {
  std::ostringstream output;
  output << year << "-" << std::setw(2) << std::setfill('0') << month;
  return output.str();
}

}  // namespace

auto StandardReportAssembler::FromMonthly(const MonthlyReportData& data)
    -> StandardReport {
  StandardReport report;
  report.report_type = "monthly";
  report.generated_at_utc = now_utc_iso8601();
  report.period_start = month_to_text(data.year, data.month);
  report.period_end = report.period_start;
  report.remark = data.remark;
  report.data_found = data.data_found;
  report.total_income = data.total_income;
  report.total_expense = data.total_expense;
  report.balance = data.balance;

  for (const auto& [parent_name, parent_data] : data.aggregated_data) {
    StandardCategoryItem parent_item;
    parent_item.name = parent_name;
    parent_item.total = parent_data.parent_total;

    for (const auto& [sub_name, sub_data] : parent_data.sub_categories) {
      StandardSubCategoryItem sub_item;
      sub_item.name = sub_name;
      sub_item.subtotal = sub_data.sub_total;

      for (const auto& tx : sub_data.transactions) {
        StandardTransactionItem tx_item;
        tx_item.parent_category = tx.parent_category;
        tx_item.sub_category = tx.sub_category;
        tx_item.transaction_type = tx.transaction_type;
        tx_item.description = tx.description;
        tx_item.source = tx.source;
        tx_item.comment = tx.comment;
        tx_item.amount = tx.amount;
        sub_item.transactions.push_back(std::move(tx_item));
      }

      parent_item.sub_categories.push_back(std::move(sub_item));
    }

    report.categories.push_back(std::move(parent_item));
  }

  return report;
}

auto StandardReportAssembler::FromYearly(const YearlyReportData& data)
    -> StandardReport {
  StandardReport report;
  report.report_type = "yearly";
  report.generated_at_utc = now_utc_iso8601();
  report.period_start = month_to_text(data.year, 1);
  report.period_end = month_to_text(data.year, 12);
  report.data_found = data.data_found;
  report.total_income = data.total_income;
  report.total_expense = data.total_expense;
  report.balance = data.balance;

  for (const auto& [month, summary] : data.monthly_summary) {
    StandardMonthlySummaryItem month_item;
    month_item.month = month;
    month_item.income = summary.income;
    month_item.expense = summary.expense;
    month_item.balance = summary.income - summary.expense;
    report.monthly_summary.push_back(std::move(month_item));
  }

  return report;
}
