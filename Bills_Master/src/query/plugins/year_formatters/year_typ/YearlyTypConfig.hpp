// query/plugins/year_formatters/year_typ/YearlyTypConfig.hpp
#ifndef YEARLY_TYP_CONFIG_HPP
#define YEARLY_TYP_CONFIG_HPP

#include <string>
#include <cstdint>

// 子结构体，用于组织年度报告的所有本地化文本标签
struct YearlyTypLabels {
    std::string year_suffix = "年 "; // 新增:用于连接年份和标题后缀的文本
    std::string report_title_suffix = "消费总览";
    std::string overview_section_title = "年度总览";
    std::string grand_total = "年度总支出";
    std::string monthly_breakdown_section_title = "每月支出";
    std::string no_data_found_prefix = "未找到 ";
    std::string no_data_found_suffix = " 年的任何数据。";
};

// 最终的、专为年度报告设计的配置结构体
struct YearlyTypConfig {
    // ================== 核心文档样式 ==================
    std::string font_family = "Noto Serif SC";
    uint8_t font_size_pt = 12;
    std::string author = "camellia";

    // ================== 格式化符号 ==================
    std::string currency_symbol = "CNY";
    uint8_t decimal_precision = 2;

    // ================== 文本标签集 ==================
    YearlyTypLabels labels;
};

#endif // YEARLY_TYP_CONFIG_HPP