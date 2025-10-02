// query/plugins/month_formatters/month_typ/MonthTypConfig.hpp
#ifndef MONTH_TYP_CONFIG_HPP
#define MONTH_TYP_CONFIG_HPP

#include <string>
#include <cstdint>

// 子结构体，用于组织所有本地化文本标签
struct MonthTypLabels {
    // --- 日期连接符 ---
    std::string year_suffix = "年"; // 用于年份后
    std::string month_suffix = "月 "; // 用于月份后 (注意末尾的空格)

    // --- 标题和标签 ---
    std::string report_title_suffix = "消费报告";
    std::string grand_total = "总支出";
    std::string remark = "备注";
    std::string category_total = "总计";
    std::string sub_category_total = "小计";
    std::string percentage_share = "占比";
    std::string no_data_found = "未找到该月的任何数据。";
};

// 最终的、专为月度报告设计的配置结构体
struct MonthTypConfig {
    // ================== 核心文档样式 ==================
    std::string font_family = "Noto Serif SC";
    uint8_t font_size_pt = 12;
    std::string author = "camellia";

    // ================== 格式化符号 ==================
    std::string currency_symbol = "CNY";
    uint8_t decimal_precision = 2;
    std::string percentage_symbol = "%";

    // ================== 文本标签集 ==================
    MonthTypLabels labels;
};

#endif // MONTH_TYP_CONFIG_HPP