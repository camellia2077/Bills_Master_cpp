// reports/core/standard_json_markdown_renderer.cpp
#include "standard_json_markdown_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "reports/standard_json/standard_report_json_serializer.hpp"
#include "standard_report_render_support.hpp"

namespace {
namespace render_support = bills::reports::render_support;

auto SortedMonthlyCategories(const StandardReport& report)
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

auto RenderMonthly(const StandardReport& report) -> std::string {
  const std::string kPeriodLabel =
      render_support::FormatMonthlyPeriodLabel(report.period_start);

  if (!report.data_found) {
    return "未找到 " + kPeriodLabel + " 的账单记录。";
  }
  const auto categories = SortedMonthlyCategories(report);

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "\n# " << render_support::MonthlyTitleText(report.period_start)
         << "\n";
  output << "\n## 总览\n";
  output << "- 收入: CNY" << report.total_income << "\n";
  output << "- 支出: CNY" << report.total_expense << "\n";
  output << "- 结余: CNY" << report.balance << "\n";
  output << "- 备注: "
         << render_support::MonthlyRemarkOrDash(report.remark) << "\n";

  for (const auto& category : categories) {
    const double kParentPct =
        (report.total_expense != 0.0)
            ? std::abs(category.total / report.total_expense * 100.0)
            : 0.0;
    output << "\n## " << category.name << "\n";
    output << "总计:CNY" << category.total << "\n";
    output << "占比:" << kParentPct << "%\n";

    for (const auto& sub : category.sub_categories) {
      const double kSubPct =
          (category.total != 0.0) ? std::abs(sub.subtotal / category.total * 100.0)
                                  : 0.0;
      output << "\n### " << sub.name << "\n";
      output << "小计:CNY" << sub.subtotal << "(占比:" << kSubPct << "%)\n";
      for (const auto& transaction : sub.transactions) {
        output << "- CNY" << transaction.amount << " "
               << transaction.description << "\n";
      }
    }
  }
  return output.str();
}

auto RenderYearly(const StandardReport& report) -> std::string {
  const std::string kYearStr =
      (report.period_start.size() >= 4U) ? report.period_start.substr(0U, 4U)
                                         : "0000";

  if (!report.data_found) {
    return "未找到 " + kYearStr + " 年的账单记录。";
  }

  auto months = report.monthly_summary;
  std::ranges::sort(months, [](const StandardMonthlySummaryItem& left,
                               const StandardMonthlySummaryItem& right) -> bool {
    return left.month < right.month;
  });

  std::ostringstream output;
  output << std::fixed << std::setprecision(2);
  output << "\n## " << kYearStr << "年 总览\n";
  output << "- **年总收入:** " << report.total_income << " CNY\n";
  output << "- **年总支出:** " << report.total_expense << " CNY\n";
  output << "- **年终结余:** " << report.balance << " CNY\n";
  output << "\n## 每月明细\n\n";
  output << "| 月份 | 收入 (CNY) | 支出 (CNY) | 结余 (CNY) |\n";
  output << "| :--- | :--- | :--- | :--- |\n";
  for (const auto& month_item : months) {
    output << "| " << kYearStr << "-" << std::setw(2) << std::setfill('0')
           << month_item.month << " | " << month_item.income << " | "
           << month_item.expense << " | "
           << (month_item.income + month_item.expense) << " |\n";
  }

  return output.str();
}

}  // namespace

auto StandardJsonMarkdownRenderer::render(const StandardReport& standard_report)
    -> std::string {
  if (standard_report.report_type == "monthly") {
    return RenderMonthly(standard_report);
  }
  if (standard_report.report_type == "yearly") {
    return RenderYearly(standard_report);
  }

  throw std::runtime_error("Unsupported report_type in standard report JSON.");
}

auto StandardJsonMarkdownRenderer::render(const std::string& standard_report_json)
    -> std::string {
  return render(StandardReportJsonSerializer::FromString(standard_report_json));
}
