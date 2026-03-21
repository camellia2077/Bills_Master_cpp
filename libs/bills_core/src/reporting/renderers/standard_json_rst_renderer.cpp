#include "standard_json_rst_renderer.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "reporting/standard_report/standard_report_dto.hpp"
#include "standard_report_render_support.hpp"

namespace {
namespace render_support = bills::core::reporting::render_support;

auto render_monthly(const StandardReport& report) -> std::string {
  const std::string period_label =
      render_support::FormatMonthlyPeriodLabel(report.period_start);
  const std::string title = render_support::MonthlyTitleText(report.period_start);

  if (!report.data_found) {
    return "未找到 " + period_label + " 的任何数据。\n";
  }

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << title << "\n";
  output << std::string(title.length() * 2, '=') << "\n\n";
  output << "总览\n";
  output << "----\n\n";
  output << "- 收入: CNY" << report.total_income << "\n";
  output << "- 支出: CNY" << report.total_expense << "\n";
  output << "- 结余: CNY" << report.balance << "\n";
  output << "- 备注: " << render_support::MonthlyRemarkOrDash(report.remark)
         << "\n\n";

  for (const auto& category : report.categories) {
    output << category.name << "\n";
    output << std::string(category.name.length() * 2, '-') << "\n";
    const double parent_pct =
        report.total_expense != 0.0
            ? std::abs(category.total / report.total_expense * 100.0)
            : 0.0;
    output << "总计: CNY" << category.total << "\n";
    output << "占总支出: " << parent_pct << "%\n\n";

    for (const auto& sub_category : category.sub_categories) {
      output << sub_category.name << "\n";
      output << std::string(sub_category.name.length() * 2, '~') << "\n";
      const double sub_pct =
          category.total != 0.0
              ? std::abs(sub_category.subtotal / category.total * 100.0)
              : 0.0;
      output << "小计: CNY" << sub_category.subtotal << " (占该类: " << sub_pct
             << "%)\n\n";
      for (const auto& transaction : sub_category.transactions) {
        output << "- CNY" << transaction.amount << " " << transaction.description
               << "\n";
      }
      output << "\n";
    }
  }

  return output.str();
}

auto render_yearly(const StandardReport& report) -> std::string {
  const std::string year_text =
      report.period_start.size() >= 4U ? report.period_start.substr(0U, 4U)
                                       : "0000";
  if (!report.data_found) {
    return "未找到 " + year_text + " 年的任何数据。\n";
  }

  const std::string title = year_text + "年 消费总览";
  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << title << "\n";
  output << std::string(title.length() * 2, '=') << "\n\n";
  output << "**年总收入:** CNY" << report.total_income << "\n";
  output << "**年总支出:** CNY" << report.total_expense << "\n";
  output << "**年终结余:** CNY" << report.balance << "\n\n";
  output << ".. list-table:: 每月明细\n";
  output << "   :widths: 15 25 25 25\n";
  output << "   :header-rows: 1\n\n";
  output << "   * - 月份\n";
  output << "     - 收入\n";
  output << "     - 支出\n";
  output << "     - 结余\n";
  for (const auto& month_item : report.monthly_summary) {
    output << "   * - " << year_text << "-" << std::setw(2)
           << std::setfill('0') << month_item.month << "\n";
    output << "     - CNY " << month_item.income << "\n";
    output << "     - CNY " << month_item.expense << "\n";
    output << "     - CNY " << (month_item.income + month_item.expense) << "\n";
  }
  return output.str();
}

}  // namespace

auto StandardJsonRstRenderer::render(const StandardReport& standard_report)
    -> std::string {
  if (standard_report.report_type == "monthly") {
    return render_monthly(standard_report);
  }
  if (standard_report.report_type == "yearly") {
    return render_yearly(standard_report);
  }
  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}
