// reports/plugins/year_formatters/year_tex/YearlyTexConfig.hpp
#ifndef YEARLY_TEX_CONFIG_HPP
#define YEARLY_TEX_CONFIG_HPP

#include <string>

struct YearlyTexConfig {
    // ... 元数据和页面布局保持不变 ...
    std::string author = "BillsMaster";
    std::string title_suffix = "年 消费总览";
    std::string document_class = "article";
    std::string paper_size = "a4paper";
    std::string margin = "1in";
    std::string main_font = "Noto Serif SC";
    std::string cjk_font = "Noto Serif SC";
    int base_font_size = 12;

    // --- 【核心修改】: 更新文本标签 ---
    std::string summary_section_title = "年度总览";
    std::string yearly_income_label = "年总收入:";
    std::string yearly_expense_label = "年总支出:";
    std::string yearly_balance_label = "年终结余:";
    std::string monthly_breakdown_title = "每月明细";
    std::string year_month_separator = "-";
    // --- 修改结束 ---
};

#endif // YEARLY_TEX_CONFIG_HPP