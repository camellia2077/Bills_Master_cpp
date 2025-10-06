// reports/plugins/month_formatters/month_rst/MonthRstConfig.hpp
#ifndef MONTH_RST_CONFIG_HPP
#define MONTH_RST_CONFIG_HPP

#include <string>

struct MonthRstConfig {
    // --- 【核心修改】: 更新元数据标签 ---
    std::string report_title_suffix = "月 消费报告";
    std::string income_prefix = "**总收入:** ";
    std::string expense_prefix = "**总支出:** ";
    std::string balance_prefix = "**结余:** ";
    std::string remark_prefix = "**备注:** ";
    // --- 修改结束 ---

    // ... 其他配置保持不变 ...
    std::string not_found_msg_part1 = "未找到 ";
    std::string not_found_msg_part2 = "年";
    std::string not_found_msg_part3 = "月的任何数据。\n";
    std::string parent_total_label = "总计:";
    std::string parent_percentage_label = "占比:";
    std::string sub_total_label = "小计:";
    std::string sub_percentage_label_prefix = "(占比:";
    std::string sub_percentage_label_suffix = "%)";
    std::string currency_symbol = "CNY";
    std::string percentage_symbol = "%";
    char title_char = '=';
    char parent_char = '-';
    char sub_char = '^';
    char transaction_char = '*';
    int precision = 2;
};

#endif // MONTH_RST_CONFIG_HPP