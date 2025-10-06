// reports/plugins/year_formatters/year_typ/YearlyTypConfig.hpp
#ifndef YEARLY_TYP_CONFIG_HPP
#define YEARLY_TYP_CONFIG_HPP

#include <string>
#include <cstdint>

struct YearlyTypLabels {
    std::string year_suffix = "年 ";
    std::string report_title_suffix = "消费总览";
    std::string overview_section_title = "年度总览";
    // --- 【核心修改】: 更新文本标签 ---
    std::string yearly_income = "年总收入";
    std::string yearly_expense = "年总支出";
    std::string yearly_balance = "年终结余";
    std::string monthly_breakdown_section_title = "每月明细";
    std::string monthly_income = "收入";
    std::string monthly_expense = "支出";
    // --- 修改结束 ---
    std::string no_data_found_prefix = "未找到 ";
    std::string no_data_found_suffix = " 年的任何数据。";
};

struct YearlyTypConfig {
    // ... 其他配置保持不变 ...
    std::string font_family = "Noto Serif SC";
    uint8_t font_size_pt = 12;
    std::string author = "camellia";
    std::string currency_symbol = "CNY";
    uint8_t decimal_precision = 2;
    YearlyTypLabels labels;
};

#endif // YEARLY_TYP_CONFIG_HPP