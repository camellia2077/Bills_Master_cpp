#include "standard_json_typst_renderer.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "reporting/standard_report/standard_report_dto.hpp"
#include "standard_report_render_support.hpp"

namespace {
namespace render_support = bills::core::reporting::render_support;

auto escape_typst(const std::string& input) -> std::string {
  std::string output;
  output.reserve(input.size());
  for (const char ch : input) {
    switch (ch) {
      case '\\':
      case '#':
      case '[':
      case ']':
      case '*':
      case '_':
      case '`':
      case '$':
      case '<':
      case '>':
      case '@':
        output.push_back('\\');
        output.push_back(ch);
        break;
      default:
        output.push_back(ch);
        break;
    }
  }
  return output;
}

auto render_monthly(const StandardReport& report) -> std::string {
  const std::string period_label =
      render_support::FormatMonthlyPeriodLabel(report.period_start);

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "#set text(font: \"Noto Serif SC\")\n";
  if (!report.data_found) {
    output << "= " << render_support::MonthlyTitleText(report.period_start)
           << "\n\n";
    output << "未找到 " << period_label << " 的任何数据。\n";
    return output.str();
  }

  output << "= " << render_support::MonthlyTitleText(report.period_start) << "\n\n";
  output << "== 总览\n";
  output << "- 收入: CNY" << report.total_income << "\n";
  output << "- 支出: CNY" << report.total_expense << "\n";
  output << "- 结余: CNY" << report.balance << "\n";
  output << "- 备注: "
         << escape_typst(render_support::MonthlyRemarkOrDash(report.remark))
         << "\n";

  for (const auto& category : report.categories) {
    const double parent_pct =
        report.total_expense != 0.0
            ? std::abs(category.total / report.total_expense * 100.0)
            : 0.0;
    output << "\n== " << escape_typst(category.name) << "\n";
    output << "*总计:* CNY" << category.total << "  \\\n";
    output << "*占总支出:* " << parent_pct << "%\n";

    for (const auto& sub_category : category.sub_categories) {
      const double sub_pct =
          category.total != 0.0
              ? std::abs(sub_category.subtotal / category.total * 100.0)
              : 0.0;
      output << "\n=== " << escape_typst(sub_category.name) << "\n";
      output << "*小计:* CNY" << sub_category.subtotal << " (占该类: " << sub_pct
             << "%)\n";
      for (const auto& transaction : sub_category.transactions) {
        output << "- CNY" << transaction.amount << " "
               << escape_typst(transaction.description) << "\n";
      }
    }
  }
  return output.str();
}

auto render_yearly(const StandardReport& report) -> std::string {
  const std::string year_text =
      report.period_start.size() >= 4U ? report.period_start.substr(0U, 4U)
                                       : "0000";
  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "#set text(font: \"Noto Serif SC\")\n";
  output << "= " << year_text << "年 消费总览\n\n";
  if (!report.data_found) {
    output << "未找到 " << year_text << " 年的任何数据。\n";
    return output.str();
  }

  output << "*年总收入:* CNY" << report.total_income << "  \\\n";
  output << "*年总支出:* CNY" << report.total_expense << "  \\\n";
  output << "*年终结余:* CNY" << report.balance << "\n\n";
  output << "#table(\n";
  output << "  columns: 4,\n";
  output << "  [月份], [收入], [支出], [结余],\n";
  for (const auto& month_item : report.monthly_summary) {
    output << "  [" << year_text << "-" << std::setw(2) << std::setfill('0')
           << month_item.month << "], ";
    output << "[CNY " << month_item.income << "], ";
    output << "[CNY " << month_item.expense << "], ";
    output << "[CNY " << (month_item.income + month_item.expense) << "],\n";
  }
  output << ")\n";
  return output.str();
}

}  // namespace

auto StandardJsonTypstRenderer::render(const StandardReport& standard_report)
    -> std::string {
  if (standard_report.report_type == "monthly") {
    return render_monthly(standard_report);
  }
  if (standard_report.report_type == "yearly") {
    return render_yearly(standard_report);
  }
  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}
