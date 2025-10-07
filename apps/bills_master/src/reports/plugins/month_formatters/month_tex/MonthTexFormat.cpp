// reports/plugins/month_formatters/month_tex/MonthTexFormat.cpp

#include "MonthTexFormat.hpp"
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath> // For std::abs

MonthTexFormat::MonthTexFormat(const MonthTexReport& config) : m_config(config) {}

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

std::string MonthTexFormat::get_no_data_message(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << "\\documentclass[" << m_config.base_font_size << "pt]{" << m_config.document_class << "}\n";
    ss << "\\usepackage{fontspec}\n";
    ss << "\\usepackage[nofonts]{ctex}\n";
    ss << "\\setmainfont{" << m_config.main_font << "}\n";
    ss << "\\setCJKmainfont{" << m_config.cjk_font << "}\n\n";
    ss << "\\begin{document}\n";
    ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
    ss << "\\end{document}\n";
    return ss.str();
}

std::string MonthTexFormat::generate_header(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << "\\documentclass[" << m_config.base_font_size << "pt]{" << m_config.document_class << "}\n";
    ss << "\\usepackage[" << m_config.paper_size << ", margin=" << m_config.margin << "]{geometry}\n";
    ss << "\\usepackage{fontspec}\n";
    ss << "\\usepackage[nofonts]{ctex}\n\n";
    ss << "% --- Font Settings from Config ---\n";
    ss << "\\setmainfont{" << m_config.main_font << "}\n";
    ss << "\\setCJKmainfont{" << m_config.cjk_font << "}\n\n";
    ss << "\\usepackage{titlesec}\n";
    ss << "\\titleformat{\\section}{" << m_config.section_format_cmd << "}{\\thesection}{1em}{}\n";
    ss << "\\titleformat{\\subsection}{" << m_config.subsection_format_cmd << "}{\\thesubsection}{1em}{}\n\n";
    ss << "\\title{" << data.year << "年" << data.month << "月 " << escape_latex(m_config.title_suffix) << "}\n";
    ss << "\\author{" << escape_latex(m_config.author) << "}\n";
    ss << "\\date{\\today}\n\n";
    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";
    return ss.str();
}

std::string MonthTexFormat::generate_summary(const MonthlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "% --- Summary Section from Config ---\n";
    ss << "\\vspace{1em}\n";
    ss << "\\hrulefill\n";
    ss << "\\begin{center}\n";
    ss << "    {" << m_config.summary_title_format_cmd << " " << escape_latex(m_config.summary_title) << "}\\par\\vspace{1em}\n";
    ss << "    {" << m_config.summary_body_format_cmd << "\n";
    ss << "    \\textbf{" << escape_latex(m_config.income_label) << "} CNY" << data.total_income << "\\\\\n";
    ss << "    \\textbf{" << escape_latex(m_config.expense_label) << "} CNY" << data.total_expense << "\\\\\n";
    ss << "    \\textbf{" << escape_latex(m_config.balance_label) << "} CNY" << data.balance << "\\\\\n";
    ss << "    \\textbf{" << escape_latex(m_config.remark_label) << "} " << escape_latex(data.remark) << "\n";
    ss << "    }\n";
    ss << "\\end{center}\n";
    ss << "\\hrulefill\n\n";
    return ss.str();
}

std::string MonthTexFormat::generate_body(const MonthlyReportData& data, const std::vector<std::pair<std::string, ParentCategoryData>>& sorted_parents) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);

    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.total_expense != 0) ? (parent_data.parent_total / data.total_expense) * 100.0 : 0.0;

        ss << "\\section*{" << escape_latex(parent_name) << "}\n";
        ss << escape_latex(m_config.parent_total_label) << "CNY" << parent_data.parent_total
           << " \t (" << escape_latex(m_config.parent_percent_label) << " " << std::abs(parent_percentage) << "\\%)\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total != 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            ss << "\\subsection*{" << escape_latex(sub_name) << "}\n";
            ss << "\\textbf{" << escape_latex(m_config.sub_total_label) << "} CNY" << sub_data.sub_total
               << " (" << escape_latex(m_config.sub_percent_label) << " " << std::abs(sub_percentage) << "\\%)\n";

            ss << "\\begin{itemize}\n";
            for (const auto& t : sub_data.transactions) {
                ss << "    \\item CNY" << t.amount << " " << escape_latex(m_config.transaction_separator) << " " << escape_latex(t.description) << "\n";
            }
            ss << "\\end{itemize}\n";
        }
    }
    return ss.str();
}

std::string MonthTexFormat::generate_footer(const MonthlyReportData& data) const {
    return "\\end{document}\n";
}

extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IMonthReportFormatter* create_tex_month_formatter() {
        return new MonthTexFormat();
    }
}
