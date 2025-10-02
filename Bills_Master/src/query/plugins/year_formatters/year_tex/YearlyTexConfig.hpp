// query/plugins/year_formatters/year_tex/YearlyTexConfig.hpp
#ifndef YEARLY_TEX_CONFIG_HPP
#define YEARLY_TEX_CONFIG_HPP

#include <string>

/**
 * @struct YearlyTexConfig
 * @brief 用于封装所有年度 LaTeX 报告生成选项的配置结构体。
 *
 * 通过实例化并修改这个结构体，用户可以自定义年度报告的
 * 字体、布局和静态文本标签，而无需修改核心的 C++ 格式化逻辑。
 */
struct YearlyTexConfig {
    // ===================================================================
    // 文档元数据 (Document Metadata)
    // ===================================================================
    std::string author = "BillsMaster";
    std::string title_suffix = "年 消费总览";

    // ===================================================================
    // 页面布局 (Page Layout)
    // ===================================================================
    std::string document_class = "article";
    std::string paper_size = "a4paper";
    std::string margin = "1in";

    // ===================================================================
    // 字体设置 (Font Settings)
    // ===================================================================
    std::string main_font = "Noto Serif SC";
    std::string cjk_font = "Noto Serif SC";
    int base_font_size = 12;

    // ===================================================================
    // 文本标签 (Text Labels for Internationalization - i18n)
    // ===================================================================
    std::string summary_section_title = "年度总览";
    std::string grand_total_label = "年度总支出:";
    std::string monthly_breakdown_title = "每月支出";
    std::string year_month_separator = "-";
};

#endif // YEARLY_TEX_CONFIG_HPP