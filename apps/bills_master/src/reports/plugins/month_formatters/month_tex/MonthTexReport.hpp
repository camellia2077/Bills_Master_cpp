// reports/plugins/month_formatters/month_tex/MonthTexReport.hpp

#ifndef MONTH_TEX_REPORT_HPP
#define MONTH_TEX_REPORT_HPP

#include <string>

struct MonthTexReport {
    // ... 其他配置保持不变 ...
    std::string author = "BillsMaster";
    std::string title_suffix = "消费报告";
    std::string document_class = "article";
    std::string paper_size = "a4paper";
    std::string margin = "1in";
    std::string main_font = "Noto Serif SC";
    std::string cjk_font = "Noto Serif SC";
    int base_font_size = 12;
    std::string section_format_cmd = "\\Large\\bfseries";
    std::string subsection_format_cmd = "\\large\\bfseries";
    std::string summary_title_format_cmd = "\\Large\\bfseries";
    std::string summary_body_format_cmd = "\\large";

    // --- 【核心修改】: 更新摘要部分的文本标签 ---
    std::string summary_title = "摘要";
    std::string income_label = "总收入：";
    std::string expense_label = "总支出：";
    std::string balance_label = "结余：";
    std::string remark_label = "备注：";
    // --- 修改结束 ---
    std::string parent_total_label = "总计：";
    std::string parent_percent_label = "占总支出:";
    std::string sub_total_label = "小计：";
    std::string sub_percent_label = "占该类:";
    std::string transaction_separator = "---";
};

#endif // MONTH_TEX_REPORT_HPP