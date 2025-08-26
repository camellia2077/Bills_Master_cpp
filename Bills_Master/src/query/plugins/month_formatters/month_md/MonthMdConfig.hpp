// MonthMdConfig.hpp
#ifndef MONTH_MD_FORMAT_CONFIG_H
#define MONTH_MD_FORMAT_CONFIG_H

#include <string>

// 结构体:用于存储Markdown格式化报告的所有内容标签和格式设置
struct MonthMdConfig {
    // --- 报告元数据标签(不含'#') ---
    std::string date_label = "DATE:";
    std::string total_label = "TOTAL:";
    std::string remark_label = "REMARK:";

    // --- 文本标签 ---
    std::string not_found_msg_part1 = "\n未找到 ";
    std::string not_found_msg_part2 = "年";
    std::string not_found_msg_part3 = "月的任何数据。\n";
    std::string parent_total_label = "总计:";
    std::string parent_percentage_label = "占比:";
    std::string sub_total_label = "小计:";
    std::string sub_percentage_label_prefix = "(占比:";
    std::string sub_percentage_label_suffix = "%)";

    // --- 符号 ---
    std::string currency_symbol = "CNY"; // 不建议使用¥ 因为字体里面可能没有 导致无法正确渲染
    std::string percentage_symbol = "%";

    // --- 格式控制 ---
    int precision = 2;
    int month_width = 2;
    char fill_char = '0';
};

#endif // MONTH_MD_FORMAT_CONFIG_H