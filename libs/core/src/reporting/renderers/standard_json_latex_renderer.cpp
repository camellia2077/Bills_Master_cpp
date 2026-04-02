#include "standard_json_latex_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "reporting/standard_report/standard_report_json_serializer.hpp"
#include "standard_report_render_support.hpp"

namespace {
namespace render_support = bills::core::reporting::render_support;

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

auto sorted_monthly_categories(const StandardReport& report)
    -> std::vector<StandardCategoryItem> {
  auto categories = report.categories;
  for (auto& category : categories) {
    for (auto& sub_category : category.sub_categories) {
      std::ranges::sort(sub_category.transactions,
                        [](const StandardTransactionItem& left,
                           const StandardTransactionItem& right) -> bool {
                          return left.amount > right.amount;
                        });
    }
  }
  std::ranges::sort(categories, [](const StandardCategoryItem& left,
                                   const StandardCategoryItem& right) -> bool {
    return left.total > right.total;
  });
  return categories;
}

auto render_monthly_remark(const std::string& remark) -> std::string {
  const auto lines = render_support::SplitRemarkLinesOrDash(remark);
  std::ostringstream output;
  output << escape_latex(lines.front());
  for (std::size_t index = 1U; index < lines.size(); ++index) {
    output << "\\\\\n    " << escape_latex(lines[index]);
  }
  return output.str();
}

auto render_monthly(const StandardReport& report) -> std::string {
  const std::string period_label =
      render_support::FormatMonthlyPeriodLabel(report.period_start);

  if (!report.data_found) {
    std::ostringstream output;
    output << "\\documentclass[12pt]{article}\n";
    output << "\\usepackage{fontspec}\n";
    output << "\\usepackage[nofonts]{ctex}\n";
    output << "\\setmainfont{Noto Serif SC}\n";
    output << "\\setCJKmainfont{Noto Serif SC}\n\n";
    output << "\\begin{document}\n";
    output << "未找到 " << period_label << " 的任何数据。\n";
    output << "\\end{document}\n";
    return output.str();
  }

  const auto categories = sorted_monthly_categories(report);

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
  output << "\\title{"
         << escape_latex(render_support::MonthlyTitleText(report.period_start))
         << "}\n";
  output << "\\author{BillsMaster}\n";
  output << "\\date{\\today}\n\n";
  output << "\\begin{document}\n";
  output << "\\maketitle\n\n";
  output << "% --- Overview Section ---\n";
  output << "\\vspace{1em}\n";
  output << "\\hrulefill\n";
  output << "\\begin{center}\n";
  output << "    {\\Large\\bfseries 总览}\\par\\vspace{1em}\n";
  output << "    {\\large\n";
  output << "    \\textbf{收入：} CNY" << report.total_income << "\\\\\n";
  output << "    \\textbf{支出：} CNY" << report.total_expense << "\\\\\n";
  output << "    \\textbf{结余：} CNY" << report.balance << "\\\\\n";
  output << "    \\textbf{备注：} " << render_monthly_remark(report.remark)
         << "\n";
  output << "    }\n";
  output << "\\end{center}\n";
  output << "\\hrulefill\n\n";

  for (const auto& category : categories) {
    const double parent_pct =
        (report.total_expense != 0.0)
            ? std::abs(category.total / report.total_expense * 100.0)
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

auto render_yearly(const StandardReport& report) -> std::string {
  const std::string year_text =
      (report.period_start.size() >= 4U) ? report.period_start.substr(0U, 4U)
                                         : "0000";

  if (!report.data_found) {
    return "未找到 " + year_text + " 年的任何数据。\n";
  }

  auto months = report.monthly_summary;
  std::ranges::sort(months, [](const StandardMonthlySummaryItem& left,
                               const StandardMonthlySummaryItem& right) -> bool {
    return left.month < right.month;
  });

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
  output << "    \\item \\textbf{年总收入:} CNY" << report.total_income << "\n";
  output << "    \\item \\textbf{年总支出:} CNY" << report.total_expense << "\n";
  output << "    \\item \\textbf{年终结余:} CNY" << report.balance << "\n";
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
           << " & CNY " << (item.income + item.expense) << " \\\\\n";
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
  if (standard_report.report_type == "monthly") {
    return render_monthly(standard_report);
  }
  if (standard_report.report_type == "yearly") {
    return render_yearly(standard_report);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}

auto StandardJsonLatexRenderer::render(const std::string& standard_report_json)
    -> std::string {
  return render(StandardReportJsonSerializer::FromString(standard_report_json));
}
