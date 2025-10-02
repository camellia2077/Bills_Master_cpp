// query/plugins/month_formatters/month_tex/MonthTexReport.hpp

#ifndef MONTH_TEX_REPORT_HPP
#define MONTH_TEX_REPORT_HPP

#include <string>

/**
 * @struct MonthTexReport
 * @brief 一个用于封装所有 LaTeX 报告生成选项的配置结构体。
 *
 * 这个结构体的目的是将格式化逻辑 (在 MonthTexFormat.cpp 中) 与
 * 外观配置分离。通过修改这个结构体的实例，用户可以深度自定义
 * 输出的 PDF 报告，而无需修改任何 C++ 编译逻辑。
 *
 * 所有成员都提供了合理的默认值，以生成一份标准的中文报告。
 */
struct MonthTexReport {
    // ===================================================================
    // 文档元数据 (Document Metadata)
    // ===================================================================
    std::string author = "BillsMaster";
    std::string title_suffix = "消费报告";

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
    // 元素格式化命令 (Element Formatting Commands)
    // ===================================================================
    // 这里可以直接填入 LaTeX 命令字符串，以获得最大的灵活性。
    // 例如，你可以使用 "\Large\bfseries", "\fontsize{16}{20}\selectfont" 等。
    std::string section_format_cmd = "\\Large\\bfseries";
    std::string subsection_format_cmd = "\\large\\bfseries";
    std::string summary_title_format_cmd = "\\Large\\bfseries";
    std::string summary_body_format_cmd = "\\large";

    // ===================================================================
    // 文本标签 (Text Labels for Internationalization - i18n)
    // ===================================================================
    // 通过修改这些标签，可以轻松生成不同语言的报告。
    std::string summary_title = "摘要";
    std::string grand_total_label = "总支出：";
    std::string remark_label = "备注：";
    std::string parent_total_label = "总计：";
    std::string parent_percent_label = "占总支出:";
    std::string sub_total_label = "小计：";
    std::string sub_percent_label = "占该类:";
    std::string transaction_separator = "---";
};

#endif // MONTH_TEX_REPORT_HPP