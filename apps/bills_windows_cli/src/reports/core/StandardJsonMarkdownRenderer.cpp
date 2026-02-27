#include "StandardJsonMarkdownRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace {
using Json = nlohmann::json;

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

auto strip_month_hyphen(const std::string& period_start) -> std::string {
  std::string output;
  output.reserve(period_start.size());
  for (const char ch : period_start) {
    if (ch != '-') {
      output.push_back(ch);
    }
  }
  return output;
}

auto render_monthly(const Json& root) -> std::string {
  const Json& scope = root.at("scope");
  const Json& summary = root.at("summary");
  const Json& items = root.at("items");

  const bool data_found = scope.value("data_found", false);
  const std::string period_start = scope.value("period_start", "");
  const std::string remark = scope.value("remark", "");
  const double total_income = summary.value("total_income", 0.0);
  const double total_expense = summary.value("total_expense", 0.0);
  const double balance = summary.value("balance", 0.0);

  if (!data_found) {
    return "未找到 " + period_start + " 的账单记录。";
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
        MonthlyTransaction tx;
        tx.amount = tx_json.value("amount", 0.0);
        tx.description = tx_json.value("description", "");
        sub.transactions.push_back(std::move(tx));
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
  output << "\n# DATE:" << strip_month_hyphen(period_start) << "\n";
  output << "# INCOME:CNY" << total_income << "\n";
  output << "# EXPENSE:CNY" << total_expense << "\n";
  output << "# BALANCE:CNY" << balance << "\n";
  output << "# REMARK:" << remark << "\n";

  for (const auto& category : categories) {
    const double parent_pct =
        (total_expense != 0.0) ? std::abs(category.total / total_expense * 100.0)
                               : 0.0;
    output << "\n# " << category.name << "\n";
    output << "总计:CNY" << category.total << "\n";
    output << "占比:" << parent_pct << "%\n";

    for (const auto& sub : category.sub_categories) {
      const double sub_pct =
          (category.total != 0.0) ? std::abs(sub.subtotal / category.total * 100.0)
                                  : 0.0;
      output << "\n## " << sub.name << "\n";
      output << "小计:CNY" << sub.subtotal << "(占比:" << sub_pct << "%)\n";
      for (const auto& tx : sub.transactions) {
        output << "- CNY" << tx.amount << " " << tx.description << "\n";
      }
    }
  }
  return output.str();
}

auto render_yearly(const Json& root) -> std::string {
  const Json& scope = root.at("scope");
  const Json& summary = root.at("summary");
  const Json& items = root.at("items");

  const bool data_found = scope.value("data_found", false);
  const std::string period_start = scope.value("period_start", "");
  const std::string year_str =
      (period_start.size() >= 4U) ? period_start.substr(0U, 4U) : "0000";

  if (!data_found) {
    return "未找到 " + year_str + " 年的账单记录。";
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

  const double total_income = summary.value("total_income", 0.0);
  const double total_expense = summary.value("total_expense", 0.0);
  const double balance = summary.value("balance", 0.0);

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "\n## " << year_str << "年 总览\n";
  output << "- **年总收入:** " << total_income << " CNY\n";
  output << "- **年总支出:** " << total_expense << " CNY\n";
  output << "- **年终结余:** " << balance << " CNY\n";
  output << "\n## 每月明细\n\n";
  output << "| 月份 | 收入 (CNY) | 支出 (CNY) | 结余 (CNY) |\n";
  output << "| :--- | :--- | :--- | :--- |\n";
  for (const auto& month_item : months) {
    output << "| " << year_str << "-" << std::setw(2) << std::setfill('0')
           << month_item.month << " | " << month_item.income << " | "
           << month_item.expense << " | " << month_item.balance << " |\n";
  }

  return output.str();
}

}  // namespace

auto StandardJsonMarkdownRenderer::render(const std::string& standard_report_json)
    -> std::string {
  const Json root = Json::parse(standard_report_json);
  const std::string report_type = root.at("meta").value("report_type", "");
  if (report_type == "monthly") {
    return render_monthly(root);
  }
  if (report_type == "yearly") {
    return render_yearly(root);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}
