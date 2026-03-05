#include "standard_json_latex_renderer.hpp"

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

auto escape_latex(const std::string& input) -> std::string {
  std::string output;
  output.reserve(input.size());
  for (const char ch : input) {
    switch (ch) {
      case '&':
        output += "\\&";
        break;
      case '%':
        output += "\\%";
        break;
      case '$':
        output += "\\$";
        break;
      case '#':
        output += "\\#";
        break;
      case '_':
        output += "\\_";
        break;
      case '{':
        output += "\\{";
        break;
      case '}':
        output += "\\}";
        break;
      case '~':
        output += "\\textasciitilde{}";
        break;
      case '^':
        output += "\\textasciicircum{}";
        break;
      case '\\':
        output += "\\textbackslash{}";
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
    output << "\\documentclass[12pt]{article}\n";
    output << "\\usepackage{fontspec}\n";
    output << "\\usepackage[nofonts]{ctex}\n";
    output << "\\setmainfont{Noto Serif SC}\n";
    output << "\\setCJKmainfont{Noto Serif SC}\n\n";
    output << "\\begin{document}\n";
    output << "未找到 " << year_text << "年" << month_text << "月的任何数据。\n";
    output << "\\end{document}\n";
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
  output << "\\documentclass[12pt]{article}\n";
  output << "\\usepackage[a4paper, margin=1in]{geometry}\n";
  output << "\\usepackage{fontspec}\n";
  output << "\\usepackage[nofonts]{ctex}\n\n";
  output << "% --- Font Settings from Config ---\n";
  output << "\\setmainfont{Noto Serif SC}\n";
  output << "\\setCJKmainfont{Noto Serif SC}\n\n";
  output << "\\usepackage{titlesec}\n";
  output << "\\titleformat{\\section}{\\Large\\bfseries}{\\thesection}{1em}{}\n";
  output << "\\titleformat{\\subsection}{\\large\\bfseries}{\\thesubsection}{1em}{}\n\n";
  output << "\\title{" << year_text << "年" << month_text << "月 消费报告}\n";
  output << "\\author{BillsMaster}\n";
  output << "\\date{\\today}\n\n";
  output << "\\begin{document}\n";
  output << "\\maketitle\n\n";
  output << "% --- Summary Section from Config ---\n";
  output << "\\vspace{1em}\n";
  output << "\\hrulefill\n";
  output << "\\begin{center}\n";
  output << "    {\\Large\\bfseries 摘要}\\par\\vspace{1em}\n";
  output << "    {\\large\n";
  output << "    \\textbf{总收入：} CNY" << total_income << "\\\\\n";
  output << "    \\textbf{总支出：} CNY" << total_expense << "\\\\\n";
  output << "    \\textbf{结余：} CNY" << balance << "\\\\\n";
  output << "    \\textbf{备注：} " << escape_latex(remark) << "\n";
  output << "    }\n";
  output << "\\end{center}\n";
  output << "\\hrulefill\n\n";

  for (const auto& category : categories) {
    const double parent_pct = (total_expense != 0.0)
                                  ? std::abs(category.total / total_expense * 100.0)
                                  : 0.0;
    output << "\\section*{" << escape_latex(category.name) << "}\n";
    output << "总计：CNY" << category.total << " \t (占总支出: " << parent_pct
           << "\\%)\n\n";
    for (const auto& sub : category.sub_categories) {
      const double sub_pct = (category.total != 0.0)
                                 ? std::abs(sub.subtotal / category.total * 100.0)
                                 : 0.0;
      output << "\\subsection*{" << escape_latex(sub.name) << "}\n";
      output << "\\textbf{小计：} CNY" << sub.subtotal << " (占该类: " << sub_pct
             << "\\%)\n";
      output << "\\begin{itemize}\n";
      for (const auto& tx : sub.transactions) {
        output << "    \\item CNY" << tx.amount << " --- "
               << escape_latex(tx.description) << "\n";
      }
      output << "\\end{itemize}\n";
    }
  }

  output << "\\end{document}\n";
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
  output << "\\documentclass[12pt]{article}\n";
  output << "\\usepackage{fontspec}\n";
  output << "\\usepackage[nofonts]{ctex}\n";
  output << "\\usepackage[a4paper, margin=1in]{geometry}\n\n";
  output << "% --- Font Settings from Config ---\n";
  output << "\\setmainfont{Noto Serif SC}\n";
  output << "\\setCJKmainfont{Noto Serif SC}\n\n";
  output << "\\title{" << year_text << "年 消费总览}\n";
  output << "\\author{BillsMaster}\n";
  output << "\\date{\\today}\n\n";
  output << "\\begin{document}\n";
  output << "\\maketitle\n\n";
  output << "\\section*{年度总览}\n";
  output << "\\begin{itemize}\n";
  output << "    \\item \\textbf{年总收入:} CNY" << total_income << "\n";
  output << "    \\item \\textbf{年总支出:} CNY" << total_expense << "\n";
  output << "    \\item \\textbf{年终结余:} CNY" << balance << "\n";
  output << "\\end{itemize}\n\n";

  output << "\\section*{每月明细}\n";
  output << "\\begin{table}[h]\n";
  output << "\\centering\n";
  output << "\\begin{tabular}{|c|c|c|c|}\n";
  output << "\\hline\n";
  output << "\\textbf{月份} & \\textbf{收入} & \\textbf{支出} & "
            "\\textbf{结余} \\\\\n";
  output << "\\hline\n";
  for (const auto& item : months) {
    output << year_text << "-" << std::setw(2) << std::setfill('0') << item.month
           << " & CNY " << item.income << " & CNY " << item.expense
           << " & CNY " << item.balance << " \\\\\n";
    output << "\\hline\n";
  }
  output << "\\end{tabular}\n";
  output << "\\end{table}\n";
  output << "\\end{document}\n";

  return output.str();
}

}  // namespace

auto StandardJsonLatexRenderer::render(const StandardReport& standard_report)
    -> std::string {
  const Json root = StandardReportJsonSerializer::ToJson(standard_report);
  const std::string report_type = root.at("meta").value("report_type", "");
  if (report_type == "monthly") {
    return render_monthly(root);
  }
  if (report_type == "yearly") {
    return render_yearly(root);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}

auto StandardJsonLatexRenderer::render(const std::string& standard_report_json)
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
