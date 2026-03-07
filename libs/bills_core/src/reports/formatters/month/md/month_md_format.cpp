// reports/formatters/month/md/month_md_format.cpp

#include "month_md_format.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

MonthMdFormat::MonthMdFormat(MonthMdConfig config)
    : config(std::move(config)) {}

auto MonthMdFormat::get_no_data_message(const MonthlyReportData& data) const
    -> std::string {
  std::ostringstream output_stream;
  output_stream << config.not_found_msg_part1 << data.year
                << config.not_found_msg_part2 << data.month
                << config.not_found_msg_part3;
  return output_stream.str();
}

auto MonthMdFormat::generate_header(const MonthlyReportData& data) const
    -> std::string {
  static_cast<void>(data);
  return "";
}

auto MonthMdFormat::generate_summary(const MonthlyReportData& data) const
    -> std::string {
  std::ostringstream output_stream;
  output_stream << std::fixed << std::setprecision(config.precision);
  output_stream << "\n# " << config.date_label << data.year
                << std::setfill(config.fill_char)
                << std::setw(config.month_width) << data.month << '\n';
  output_stream << "# " << config.income_label << config.currency_symbol
                << data.total_income << '\n';
  output_stream << "# " << config.expense_label << config.currency_symbol
                << data.total_expense << '\n';
  output_stream << "# " << config.balance_label << config.currency_symbol
                << data.balance << '\n';
  output_stream << "# " << config.remark_label << data.remark << '\n';
  return output_stream.str();
}

auto MonthMdFormat::generate_body(
    const MonthlyReportData& data,
    const std::vector<std::pair<std::string, ParentCategoryData>>&
        sorted_parents) const -> std::string {
  std::ostringstream output_stream;
  output_stream << std::fixed << std::setprecision(config.precision);

  for (const auto& [parent_name, parent_data] : sorted_parents) {
    output_stream << "\n# " << parent_name << '\n';
    const double kParentPercentage =
        (data.total_expense != 0.0)
            ? (parent_data.parent_total / data.total_expense) * 100.0
            : 0.0;
    output_stream << config.parent_total_label << config.currency_symbol
                  << parent_data.parent_total << '\n';
    output_stream << config.parent_percentage_label
                  << std::abs(kParentPercentage) << config.percentage_symbol
                  << '\n';

    for (const auto& [sub_name, sub_data] : parent_data.sub_categories) {
      output_stream << "\n## " << sub_name << '\n';
      const double kSubPercentage =
          (parent_data.parent_total != 0.0)
              ? (sub_data.sub_total / parent_data.parent_total) * 100.0
              : 0.0;
      output_stream << config.sub_total_label << config.currency_symbol
                    << sub_data.sub_total << config.sub_percentage_label_prefix
                    << std::abs(kSubPercentage)
                    << config.sub_percentage_label_suffix << '\n';

      for (const auto& transaction : sub_data.transactions) {
        output_stream << "- " << config.currency_symbol << transaction.amount
                      << " " << transaction.description << '\n';
      }
    }
  }

  return output_stream.str();
}

auto MonthMdFormat::generate_footer(const MonthlyReportData& data) const
    -> std::string {
  static_cast<void>(data);
  return "";
}
