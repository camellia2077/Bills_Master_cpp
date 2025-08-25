
#include "YearTexFormat.h"
#include <sstream>
#include <iomanip>

YearTexFormat::YearTexFormat(const YearlyTexConfig& config) : m_config(config) {}

std::string YearTexFormat::escape_latex(const std::string& input) const {
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

std::string YearTexFormat::get_no_data_message(int year) const {
    return "未找到 " + std::to_string(year) + " 年的任何数据。\n";
}

std::string YearTexFormat::generate_header(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << "\\documentclass[" << m_config.base_font_size << "pt]{" << m_config.document_class << "}\n";
    ss << "\\usepackage{fontspec}\n";
    ss << "\\usepackage[nofonts]{ctex}\n";
    ss << "\\usepackage[" << m_config.paper_size << ", margin=" << m_config.margin << "]{geometry}\n\n";
    ss << "% --- Font Settings from Config ---\n";
    ss << "\\setmainfont{" << m_config.main_font << "}\n";
    ss << "\\setCJKmainfont{" << m_config.cjk_font << "}\n\n";
    ss << "\\title{" << data.year << escape_latex(m_config.title_suffix) << "}\n";
    ss << "\\author{" << escape_latex(m_config.author) << "}\n";
    ss << "\\date{\\today}\n\n";
    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";
    return ss.str();
}

std::string YearTexFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "\\section*{" << escape_latex(m_config.summary_section_title) << "}\n";
    ss << "\\begin{itemize}\n";
    ss << "    \\item \\textbf{" << escape_latex(m_config.grand_total_label) << "} CNY" << data.grand_total << "\n";
    ss << "\\end{itemize}\n\n";
    return ss.str();
}

std::string YearTexFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    ss << "\\section*{" << escape_latex(m_config.monthly_breakdown_title) << "}\n";
    ss << "\\begin{itemize}\n";
    return ss.str();
}

std::string YearTexFormat::generate_monthly_item(int year, int month, double total) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "    \\item " << year << escape_latex(m_config.year_month_separator)
       << std::setfill('0') << std::setw(2) << month
       << ":CNY" << total << "\n";
    return ss.str();
}

std::string YearTexFormat::generate_footer(const YearlyReportData& data) const {
    return "\\end{itemize}\n\n\\end{document}\n";
}


// extern "C" 代码块保持不变
extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IYearlyReportFormatter* create_tex_year_formatter() {
        return new YearTexFormat();
    }
}