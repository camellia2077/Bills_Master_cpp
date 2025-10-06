// reports/plugins/year_formatters/year_md/YearMdConfig.hpp
#ifndef YEAR_MD_CONFIG_HPP
#define YEAR_MD_CONFIG_HPP

#include <string>

struct YearMdConfig {
    // --- 【核心修改】: 更新文本标签 ---
    std::string yearly_income_label = "年总收入";
    std::string yearly_expense_label = "年总支出";
    std::string yearly_balance_label = "年终结余";
    std::string monthly_table_header_income = "收入";
    std::string monthly_table_header_expense = "支出";
    // --- 修改结束 ---

    std::string not_found_msg_part1 = "\n未找到 ";
    std::string not_found_msg_part2 = " 年的任何数据。\n";
    std::string monthly_breakdown_title = "每月明细";
    std::string monthly_table_header_month = "月份";
    std::string currency_name = "CNY";
    std::string monthly_item_date_separator = "-";
    int precision = 2;
    int month_width = 2;
    char fill_char = '0';
};

#endif // YEAR_MD_CONFIG_HPP