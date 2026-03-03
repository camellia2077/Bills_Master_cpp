#include "standard_json_typst_renderer.hpp"

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

auto escape_typst(const std::string& input) -> std::string {
  std::string output;
  output.reserve(input.size());
  for (const char ch : input) {
    switch (ch) {
      case '\\':
        output += "\\\\";
        break;
      case '*':
        output += "\\*";
        break;
      case '_':
        output += "\\_";
        break;
      case '#':
        output += "\\#";
        break;
      default:
        output += ch;
        break;
    }
  }
  return output;
}

auto parse_period_month(const std::string& period_start, int& year, int& month)
    -> bool {
  if (period_start.size() < 7U || period_start[4] != '-') {
    return false;
  }
  try {
    year = std::stoi(period_start.substr(0U, 4U));
    month = std::stoi(period_start.substr(5U, 2U));
  } catch (...) {
    return false;
  }
  return month >= 1 && month <= 12;
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

  int year = 0;
  int month = 0;
  const bool parsed_period = parse_period_month(period_start, year, month);
  const std::string year_text =
      parsed_period ? std::to_string(year) : period_start.substr(0U, 4U);
  const std::string month_text =
      parsed_period ? std::to_string(month) : period_start;

  if (!data_found) {
    std::ostringstream output;
    output << "#set document(title: \"" << year_text << "年" << month_text
           << "月 消费报告\", author: \"camellia\")\n";
    output << "#set text(font: \"Noto Serif SC\", size: 12pt)\n\n";
    output << "未找到该月的任何数据。\n";
    return output.str();
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
  output << "#set document(title: \"" << year_text << "年" << month_text
         << "月 消费报告\", author: \"camellia\")\n";
  output << "#set text(font: \"Noto Serif SC\", size: 12pt)\n\n";
  output << "= " << year_text << "年" << month_text << "月 消费报告\n\n";
  output << "*总收入:* CNY" << total_income << "\n";
  output << "*总支出:* CNY" << total_expense << "\n";
  output << "*结余:* CNY" << balance << "\n";
  output << "*备注:* " << escape_typst(remark) << "\n\n";

  for (const auto& category : categories) {
    const double parent_pct = (total_expense != 0.0)
                                  ? (category.total / total_expense) * 100.0
                                  : 0.0;
    output << "== " << escape_typst(category.name) << "\n\n";
    output << "*总计:* CNY" << category.total << "\n";
    output << "*占比:* " << std::abs(parent_pct) << "%\n\n";

    for (const auto& sub : category.sub_categories) {
      const double sub_pct = (category.total != 0.0)
                                 ? (sub.subtotal / category.total) * 100.0
                                 : 0.0;
      output << "=== " << escape_typst(sub.name) << "\n";
      output << "  *小计:* CNY" << sub.subtotal << " (占比: " << std::abs(sub_pct)
             << "%)\n";
      for (const auto& tx : sub.transactions) {
        output << "  - CNY" << tx.amount << " " << escape_typst(tx.description)
               << "\n";
      }
      output << "\n";
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
  const std::string year_text =
      (period_start.size() >= 4U) ? period_start.substr(0U, 4U) : "0000";

  if (!data_found) {
    return "未找到 " + year_text + " 年的任何数据。\n";
  }

  struct YearlyMonthItem {
    int month = 0;
    double income = 0.0;
    double expense = 0.0;
    double balance = 0.0;
  };
  std::vector<YearlyMonthItem> months;
  for (const auto& month_json : items.at("monthly_summary")) {
    YearlyMonthItem item;
    item.month = month_json.value("month", 0);
    item.income = month_json.value("income", 0.0);
    item.expense = month_json.value("expense", 0.0);
    // Align with existing yearly formatters: monthly balance is income + expense.
    item.balance = item.income + item.expense;
    months.push_back(item);
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
  output << "#set document(title: \"" << year_text
         << "年 消费总览\", author: \"camellia\")\n";
  output << "#set text(font: \"Noto Serif SC\", size: 12pt)\n\n";
  output << "= " << year_text << "年 消费总览\n\n";
  output << "== 年度总览\n\n";
  output << "  - *年总收入:* CNY" << total_income << "\n";
  output << "  - *年总支出:* CNY" << total_expense << "\n";
  output << "  - *年终结余:* CNY" << balance << "\n\n";

  output << "== 每月明细\n\n";
  output << "#table(\n";
  output << "  columns: (auto, 1fr, 1fr, 1fr),\n";
  output << "  inset: 10pt,\n";
  output << "  align: horizon,\n";
  output << "  [*月份*], [*收入*], [*支出*], [*结余*],\n";
  for (const auto& item : months) {
    output << "  [" << year_text << "-" << std::setfill('0') << std::setw(2)
           << item.month << "], "
           << "[CNY" << item.income << "], "
           << "[CNY" << item.expense << "], "
           << "[CNY" << item.balance << "],\n";
  }
  output << ")\n";
  return output.str();
}

}  // namespace

auto StandardJsonTypstRenderer::render(const std::string& standard_report_json)
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
