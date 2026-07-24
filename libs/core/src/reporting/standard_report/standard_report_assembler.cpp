// reporting/standard_report/standard_report_assembler.cpp
#include "reporting/standard_report/standard_report_assembler.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
auto NowUtcIso8601() -> std::string {
  const auto kNow = std::chrono::system_clock::now();
  const std::time_t kNowTime = std::chrono::system_clock::to_time_t(kNow);
  std::tm utc_tm {};
#ifdef _WIN32
  gmtime_s(&utc_tm, &kNowTime);
#else
  gmtime_r(&kNowTime, &utc_tm);
#endif
  std::ostringstream output;
  output << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return output.str();
}

auto MonthToText(const int kYear, const int kMonth) -> std::string {
  std::ostringstream output;
  output << kYear << "-" << std::setw(2) << std::setfill('0') << kMonth;
  return output.str();
}

auto AppendMonthlySummary(StandardReport& report,
                          const RangeReportData& data) -> void {
  for (const auto& month : data.months) {
    StandardMonthlySummaryItem month_item;
    month_item.period = MonthToText(month.year, month.month);
    month_item.income = month.total_income;
    month_item.expense = month.total_expense;
    month_item.balance = month.balance;
    report.monthly_summary.push_back(std::move(month_item));
  }
}

}  // namespace

auto StandardReportAssembler::FromMonthly(const MonthlyReportData& data)
    -> StandardReport {
  StandardReport report;
  report.report_type = "monthly";
  report.generated_at_utc = NowUtcIso8601();
  report.period_start = MonthToText(data.year, data.month);
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

      for (const auto& transaction : sub_data.transactions) {
        StandardTransactionItem transaction_item;
        transaction_item.parent_category = transaction.parent_category;
        transaction_item.sub_category = transaction.sub_category;
        transaction_item.transaction_type = transaction.transaction_type;
        transaction_item.description = transaction.description;
        transaction_item.source = transaction.source;
        transaction_item.comment = transaction.comment;
        transaction_item.amount = transaction.amount;
        sub_item.transactions.push_back(std::move(transaction_item));
      }

      parent_item.sub_categories.push_back(std::move(sub_item));
    }

    report.categories.push_back(std::move(parent_item));
  }

  return report;
}

auto StandardReportAssembler::FromYearly(const RangeReportData& data)
    -> StandardReport {
  StandardReport report;
  report.report_type = "yearly";
  report.generated_at_utc = NowUtcIso8601();
  report.period_start = data.period_start;
  report.period_end = data.period_end;
  report.data_found = data.data_found;
  report.total_income = data.total_income;
  report.total_expense = data.total_expense;
  report.balance = data.balance;
  AppendMonthlySummary(report, data);
  return report;
}

auto StandardReportAssembler::FromRange(const RangeReportData& data)
    -> StandardReport {
  StandardReport report;
  report.report_type = "range";
  report.generated_at_utc = NowUtcIso8601();
  report.period_start = data.period_start;
  report.period_end = data.period_end;
  report.data_found = data.data_found;
  report.total_income = data.total_income;
  report.total_expense = data.total_expense;
  report.balance = data.balance;
  AppendMonthlySummary(report, data);
  return report;
}
