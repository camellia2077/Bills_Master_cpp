// reports/formatters/year/md/year_md_format.cpp

#include "year_md_format.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

YearMdFormat::YearMdFormat(YearMdConfig config) : config(std::move(config)) {}

auto YearMdFormat::get_no_data_message(int year) const -> std::string {
  std::ostringstream output_stream;
  output_stream << config.not_found_msg_part1 << year
                << config.not_found_msg_part2;
  return output_stream.str();
}

auto YearMdFormat::generate_header(const YearlyReportData& data) const
    -> std::string {
  static_cast<void>(data);
  return "";
}

auto YearMdFormat::generate_summary(const YearlyReportData& data) const
    -> std::string {
  std::ostringstream output_stream;
  output_stream << std::fixed << std::setprecision(config.precision);
  output_stream << "\n## " << data.year << "骞?鎬昏\n";
  output_stream << "- **" << config.yearly_income_label << ":** "
                << data.total_income << " " << config.currency_name << "\n";
  output_stream << "- **" << config.yearly_expense_label << ":** "
                << data.total_expense << " " << config.currency_name << "\n";
  output_stream << "- **" << config.yearly_balance_label << ":** "
                << data.balance << " " << config.currency_name << "\n";
  return output_stream.str();
}

auto YearMdFormat::generate_monthly_breakdown_header() const -> std::string {
  std::ostringstream output_stream;
  output_stream << "\n## " << config.monthly_breakdown_title << "\n\n"
                << "| " << config.monthly_table_header_month << " | "
                << config.monthly_table_header_income << " ("
                << config.currency_name << ") | "
                << config.monthly_table_header_expense << " ("
                << config.currency_name << ") | "
                << config.monthly_table_header_balance << " ("
                << config.currency_name << ") |\n"
                << "| :--- | :--- | :--- | :--- |\n";
  return output_stream.str();
}

auto YearMdFormat::generate_monthly_item(int year, int month,
                                         const MonthlySummary& summary) const
    -> std::string {
  std::ostringstream output_stream;
  const double kMonthlyBalance = summary.income + summary.expense;

  output_stream << std::fixed << std::setprecision(config.precision);
  output_stream << "| " << year << config.monthly_item_date_separator
                << std::setfill(config.fill_char)
                << std::setw(config.month_width) << month << " | "
                << summary.income << " | " << summary.expense << " | "
                << kMonthlyBalance << " |\n";
  return output_stream.str();
}

auto YearMdFormat::generate_footer(const YearlyReportData& data) const
    -> std::string {
  static_cast<void>(data);
  return "";
}
