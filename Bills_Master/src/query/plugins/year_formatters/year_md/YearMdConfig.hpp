// query/plugins/year_formatters/year_md/YearMdConfig.hpp
#ifndef YEAR_MD_CONFIG_HPP
#define YEAR_MD_CONFIG_HPP

#include <string>

// 结构体:用于存储年度Markdown报告的所有内容标签和格式设置
struct YearMdConfig {
    // --- 文本标签 ---
    std::string not_found_msg_part1 = "\n未找到 ";
    std::string not_found_msg_part2 = " 年的任何数据。\n";
    
    // 用于 "2023年总支出"
    std::string yearly_total_label = "年总支出";

    // 用于 "每月支出"
    std::string monthly_breakdown_title = "每月支出";
    
    // --- 新增：为表格格式化添加的文本 ---
    std::string monthly_table_header_month = "月份";
    std::string monthly_table_header_amount = "金额";

    // --- 符号和后缀 ---
    std::string currency_name = "CNY"; // 不建议使用¥ 因为字体里面可能没有 导致无法正确渲染
    std::string monthly_item_date_separator = "-"; // 日期的连接符
    // monthly_item_value_separator 不再需要，因为表格有自己的分隔符

    // --- 格式控制 ---
    int precision = 2;
    int month_width = 2;
    char fill_char = '0';
};

#endif // YEAR_MD_CONFIG_HPP