// reports/plugins/month_formatters/month_typ/MonthTypConfig.hpp
#ifndef MONTH_TYP_CONFIG_HPP
#define MONTH_TYP_CONFIG_HPP

#include <string>
#include <cstdint>

struct MonthTypLabels {
    std::string year_suffix = "年";
    std::string month_suffix = "月 ";

    // --- 【核心修改】: 更新标题和标签 ---
    std::string report_title_suffix = "消费报告";
    std::string income_total = "总收入";
    std::string expense_total = "总支出";
    std::string balance = "结余";
    std::string remark = "备注";
    // --- 修改结束 ---
    std::string category_total = "总计";
    std::string sub_category_total = "小计";
    std::string percentage_share = "占比";
    std::string no_data_found = "未找到该月的任何数据。";
};

struct MonthTypConfig {
    // ... 其他配置保持不变 ...
    std::string font_family = "Noto Serif SC";
    uint8_t font_size_pt = 12;
    std::string author = "camellia";
    std::string currency_symbol = "CNY";
    uint8_t decimal_precision = 2;
    std::string percentage_symbol = "%";
    MonthTypLabels labels;
};

#endif // MONTH_TYP_CONFIG_HPP