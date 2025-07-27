// MonthTexFormat.cpp

#include "MonthTexFormat.h"
#include "query/month/common/ReportSorter.h"
#include <sstream>
#include <iomanip>
#include <string>

// --- 实现新的构造函数 ---
MonthTexFormat::MonthTexFormat(const MonthTexReport& config)
    : m_config(config) // 使用成员初始化列表来初始化配置
{
}

// --- escape_latex 函数保持不变 ---
std::string MonthTexFormat::escape_latex(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    for (const char c : input) {
        switch (c) {
            case '&':  output += "\\&";        break;
            case '%':  output += "\\%";        break;
            case '$':  output += "\\$";        break;
            case '#':  output += "\\#";        break;
            case '_':  output += "\\_";        break;
            case '{':  output += "\\{";        break;
            case '}':  output += "\\}";        break;
            case '~':  output += "\\textasciitilde{}"; break;
            case '^':  output += "\\textasciicircum{}"; break;
            case '\\': output += "\\textbackslash{}"; break;
            default:   output += c;            break;
        }
    }
    return output;
}

// --- format_report 函数现在使用 m_config 成员变量 ---
std::string MonthTexFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    // --- 1. 文档前导和字体设置 (使用配置变量) ---
    ss << "\\documentclass[" << m_config.base_font_size << "pt]{" << m_config.document_class << "}\n";
    ss << "\\usepackage[" << m_config.paper_size << ", margin=" << m_config.margin << "]{geometry}\n";
    ss << "\\usepackage{fontspec}\n";
    ss << "\\usepackage[nofonts]{ctex}\n\n";
    
    ss << "% --- Font Settings from Config ---\n";
    ss << "\\setmainfont{" << m_config.main_font << "}\n";
    ss << "\\setCJKmainfont{" << m_config.cjk_font << "}\n\n";

    if (!data.data_found) {
        ss << "\\begin{document}\n";
        ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        ss << "\\end{document}\n";
        return ss.str();
    }

    auto sorted_parents = ReportSorter::sort_report_data(data);

    ss << std::fixed << std::setprecision(2);
    
    // --- 文档信息 (使用配置变量) ---
    ss << "\\usepackage{titlesec}\n";
    ss << "\\titleformat{\\section}{" << m_config.section_format_cmd << "}{\\thesection}{1em}{}\n";
    ss << "\\titleformat{\\subsection}{" << m_config.subsection_format_cmd << "}{\\thesubsection}{1em}{}\n\n";
    
    ss << "\\title{" << data.year << "年" << data.month << "月 " << escape_latex(m_config.title_suffix) << "}\n";
    ss << "\\author{" << escape_latex(m_config.author) << "}\n";
    ss << "\\date{\\today}\n\n";

    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";

    // --- 摘要部分 (使用配置变量) ---
    ss << "% --- Summary Section from Config ---\n";
    ss << "\\vspace{1em}\n";
    ss << "\\hrulefill\n";
    ss << "\\begin{center}\n";
    ss << "    {" << m_config.summary_title_format_cmd << " " << escape_latex(m_config.summary_title) << "}\\par\\vspace{1em}\n";
    ss << "    {" << m_config.summary_body_format_cmd << "\n";
    ss << "    \\textbf{" << escape_latex(m_config.grand_total_label) << "} ¥" << data.grand_total << "\\\\\n";
    ss << "    \\textbf{" << escape_latex(m_config.remark_label) << "} " << escape_latex(data.remark) << "\n";
    ss << "    }\n";
    ss << "\\end{center}\n";
    ss << "\\hrulefill\n\n";

    // --- 遍历并生成内容 (使用配置变量) ---
    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        
        ss << "\\section*{" << escape_latex(parent_name) << "}\n";
        ss << escape_latex(m_config.parent_total_label) << "¥" << parent_data.parent_total 
           << " \t (" << escape_latex(m_config.parent_percent_label) << " " << parent_percentage << "\\%)\n\n";
        
        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            ss << "\\subsection*{" << escape_latex(sub_name) << "}\n";
            ss << "\\textbf{" << escape_latex(m_config.sub_total_label) << "} ¥" << sub_data.sub_total 
               << " (" << escape_latex(m_config.sub_percent_label) << " " << sub_percentage << "\\%)\n";
            
            ss << "\\begin{itemize}\n";
            for (const auto& t : sub_data.transactions) {
                ss << "    \\item ¥" << t.amount << " " << escape_latex(m_config.transaction_separator) << " " << escape_latex(t.description) << "\n";
            }
            ss << "\\end{itemize}\n";
        }
    }

    ss << "\\end{document}\n";

    return ss.str();
}