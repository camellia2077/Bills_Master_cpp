// reports/core/standard_json_markdown_renderer.cpp
#include "standard_json_markdown_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "reports/standard_json/standard_report_json_serializer.hpp"

namespace {
using Json = nlohmann::ordered_json;

struct MonthlyTransaction {
  double amount = 0.0;
  std::string description;
};

struct MonthlySubCategory {
  std::string name;
  double subtotal = 0.0;
  std::vector<MonthlyTransaction> transactions;
};

struct MonthlyCategory {
  std::string name;
  double total = 0.0;
  std::vector<MonthlySubCategory> sub_categories;
};

auto StripMonthHyphen(const std::string& period_start) -> std::string {
  std::string output;
  output.reserve(period_start.size());
  for (const char kCh : period_start) {
    if (kCh != '-') {
      output.push_back(kCh);
    }
  }
  return output;
}

auto RenderMonthly(const Json& root) -> std::string {
  const Json& scope = root.at("scope");
  const Json& summary = root.at("summary");
  const Json& items = root.at("items");

  const bool kDataFound = scope.value("data_found", false);
  const std::string kPeriodStart = scope.value("period_start", "");
  const std::string kRemark = scope.value("remark", "");
  const double kTotalIncome = summary.value("total_income", 0.0);
  const double kTotalExpense = summary.value("total_expense", 0.0);
  const double kBalance = summary.value("balance", 0.0);

  if (!kDataFound) {
    return "未找到 " + kPeriodStart + " 的账单记录。";
  }

  std::vector<MonthlyCategory> categories;
  for (const auto& category_json : items.at("categories")) {
    MonthlyCategory category;
    category.name = category_json.value("name", "");
    category.total = category_json.value("total", 0.0);

    for (const auto& sub_json : category_json.at("sub_categories")) {
      MonthlySubCategory sub;
      sub.name = sub_json.value("name", "");
      sub.subtotal = sub_json.value("subtotal", 0.0);
      for (const auto& tx_json : sub_json.at("transactions")) {
        MonthlyTransaction transaction;
        transaction.amount = tx_json.value("amount", 0.0);
        transaction.description = tx_json.value("description", "");
        sub.transactions.push_back(std::move(transaction));
      }
      std::ranges::sort(sub.transactions,
                        [](const MonthlyTransaction& left,
                           const MonthlyTransaction& right) -> bool {
                          return left.amount > right.amount;
                        });
      category.sub_categories.push_back(std::move(sub));
    }

    categories.push_back(std::move(category));
  }

  std::ranges::sort(categories, [](const MonthlyCategory& left,
                                   const MonthlyCategory& right) -> bool {
    return left.total > right.total;
  });

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "\n# DATE:" << StripMonthHyphen(kPeriodStart) << "\n";
  output << "# INCOME:CNY" << kTotalIncome << "\n";
  output << "# EXPENSE:CNY" << kTotalExpense << "\n";
  output << "# BALANCE:CNY" << kBalance << "\n";
  output << "# REMARK:" << kRemark << "\n";

  for (const auto& category : categories) {
    const double kParentPct =
        (kTotalExpense != 0.0) ? std::abs(category.total / kTotalExpense * 100.0)
                               : 0.0;
    output << "\n# " << category.name << "\n";
    output << "总计:CNY" << category.total << "\n";
    output << "占比:" << kParentPct << "%\n";

    for (const auto& sub : category.sub_categories) {
      const double kSubPct =
          (category.total != 0.0) ? std::abs(sub.subtotal / category.total * 100.0)
                                  : 0.0;
      output << "\n## " << sub.name << "\n";
      output << "小计:CNY" << sub.subtotal << "(占比:" << kSubPct << "%)\n";
      for (const auto& transaction : sub.transactions) {
        output << "- CNY" << transaction.amount << " "
               << transaction.description << "\n";
      }
    }
  }
  return output.str();
}

auto RenderYearly(const Json& root) -> std::string {
  const Json& scope = root.at("scope");
  const Json& summary = root.at("summary");
  const Json& items = root.at("items");

  const bool kDataFound = scope.value("data_found", false);
  const std::string kPeriodStart = scope.value("period_start", "");
  const std::string kYearStr =
      (kPeriodStart.size() >= 4U) ? kPeriodStart.substr(0U, 4U) : "0000";

  if (!kDataFound) {
    return "未找到 " + kYearStr + " 年的账单记录。";
  }

  struct YearlyMonthItem {
    int month = 0;
    double income = 0.0;
    double expense = 0.0;
    double balance = 0.0;
  };
  std::vector<YearlyMonthItem> months;
  for (const auto& month_json : items.at("monthly_summary")) {
    YearlyMonthItem month_item;
    month_item.month = month_json.value("month", 0);
    month_item.income = month_json.value("income", 0.0);
    month_item.expense = month_json.value("expense", 0.0);
    // Align with existing MD formatter: monthly balance is income + expense.
    month_item.balance = month_item.income + month_item.expense;
    months.push_back(month_item);
  }
  std::ranges::sort(months, [](const YearlyMonthItem& left,
                               const YearlyMonthItem& right) -> bool {
    return left.month < right.month;
  });

  const double kTotalIncome = summary.value("total_income", 0.0);
  const double kTotalExpense = summary.value("total_expense", 0.0);
  const double kBalance = summary.value("balance", 0.0);

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "\n## " << kYearStr << "年 总览\n";
  output << "- **年总收入:** " << kTotalIncome << " CNY\n";
  output << "- **年总支出:** " << kTotalExpense << " CNY\n";
  output << "- **年终结余:** " << kBalance << " CNY\n";
  output << "\n## 每月明细\n\n";
  output << "| 月份 | 收入 (CNY) | 支出 (CNY) | 结余 (CNY) |\n";
  output << "| :--- | :--- | :--- | :--- |\n";
  for (const auto& month_item : months) {
    output << "| " << kYearStr << "-" << std::setw(2) << std::setfill('0')
           << month_item.month << " | " << month_item.income << " | "
           << month_item.expense << " | " << month_item.balance << " |\n";
  }

  return output.str();
}

}  // namespace

auto StandardJsonMarkdownRenderer::render(const StandardReport& standard_report)
    -> std::string {
  const Json kRoot = StandardReportJsonSerializer::ToJson(standard_report);
  const std::string kReportType = kRoot.at("meta").value("report_type", "");
  if (kReportType == "monthly") {
    return RenderMonthly(kRoot);
  }
  if (kReportType == "yearly") {
    return RenderYearly(kRoot);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}

auto StandardJsonMarkdownRenderer::render(const std::string& standard_report_json)
    -> std::string {
  const Json kRoot = Json::parse(standard_report_json);
  const std::string kReportType = kRoot.at("meta").value("report_type", "");
  if (kReportType == "monthly") {
    return RenderMonthly(kRoot);
  }
  if (kReportType == "yearly") {
    return RenderYearly(kRoot);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}
