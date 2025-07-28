// YearTexFormat.cpp

#include "YearTexFormat.h"
#include <sstream>
#include <iomanip>

// --- 实现新的构造函数 ---
YearTexFormat::YearTexFormat(const YearlyTexConfig& config)
    : m_config(config) 
{
}

// --- 新增: 实现 escape_latex 工具函数 ---
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

std::string YearTexFormat::format_report(const YearlyReportData& data) const{
    std::stringstream ss;

    // --- 使用配置变量构建文档前导 ---
    ss << "\\documentclass[" << m_config.base_font_size << "pt]{" << m_config.document_class << "}\n";
    ss << "\\usepackage{fontspec}\n"; // 添加 fontspec 以支持 setmainfont
    ss << "\\usepackage[nofonts]{ctex}\n"; // 将 ctex 改为 nofonts 模式，手动控制字体
    ss << "\\usepackage[" << m_config.paper_size << ", margin=" << m_config.margin << "]{geometry}\n\n";

    ss << "% --- Font Settings from Config ---\n";
    ss << "\\setmainfont{" << m_config.main_font << "}\n";
    ss << "\\setCJKmainfont{" << m_config.cjk_font << "}\n\n";

    ss << "\\title{" << data.year << escape_latex(m_config.title_suffix) << "}\n";
    ss << "\\author{" << escape_latex(m_config.author) << "}\n";
    ss << "\\date{\\today}\n\n";
    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";

    if (!data.data_found) {
        ss << "未找到 " << data.year << " 年的任何数据。\n";
        ss << "\\end{document}\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);

    // --- 使用配置变量构建摘要部分 ---
    ss << "\\section*{" << escape_latex(m_config.summary_section_title) << "}\n";
    ss << "\\begin{itemize}\n";
    ss << "    \\item \\textbf{" << escape_latex(m_config.grand_total_label) << "} CNY" << data.grand_total << "\n";
    ss << "\\end{itemize}\n\n";

    // --- 使用配置变量构建每月详情 ---
    ss << "\\section*{" << escape_latex(m_config.monthly_breakdown_title) << "}\n";
    ss << "\\begin{itemize}\n";
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "    \\item " << data.year << escape_latex(m_config.year_month_separator) 
           << std::setfill('0') << std::setw(2) << month_val
           << ":CNY" << month_total << "\n";
    }
    ss << "\\end{itemize}\n\n";

    ss << "\\end{document}\n";

    return ss.str();
}

// =================================================================
// ================== 以下是为动态库添加的部分 ==================
// =================================================================
extern "C" {

    // 定义平台特定的导出宏
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    /**
     * @brief 创建 YearTexFormat 格式化器实例的工厂函数。
     * @return 指向 IYearlyReportFormatter 接口的指针。
     *
     * 这是此动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象。
     */
    PLUGIN_API IYearlyReportFormatter* create_tex_year_formatter() {
        // 创建并返回一个新的格式化器实例
        return new YearTexFormat();
    }

} // extern "C"