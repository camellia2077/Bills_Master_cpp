#include "LatexReportFormatter.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>

// 实现 LaTeX 特殊字符转义
std::string LatexReportFormatter::escape_latex(const std::string& input) {
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

std::string LatexReportFormatter::format_report(const MonthlyReportData& data) {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "\\documentclass{article}\n";
        ss << "\\usepackage{ctex}\n";
        ss << "\\begin{document}\n";
        ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        ss << "\\end{document}\n";
        return ss.str();
    }

    // --- 排序逻辑 (与原 ReportFormatter 相同) ---
    auto sorted_data = data.aggregated_data;
    for (auto& parent_pair : sorted_data) {
        for (auto& sub_pair : parent_pair.second.sub_categories) {
            std::sort(sub_pair.second.transactions.begin(), sub_pair.second.transactions.end(),
                [](const Transaction& a, const Transaction& b) { return a.amount > b.amount; });
        }
    }

    std::vector<std::pair<std::string, ParentCategoryData>> sorted_parents;
    for (const auto& pair : sorted_data) {
        sorted_parents.push_back(pair);
    }
    std::sort(sorted_parents.begin(), sorted_parents.end(),
        [](const auto& a, const auto& b) { return a.second.parent_total > b.second.parent_total; });

    // --- 构建 LaTeX 文档 ---
    ss << std::fixed << std::setprecision(2);

    // 1. 文档前导和标题
    ss << "\\documentclass[12pt]{article}\n";
    ss << "\\usepackage{ctex}\n"; // 支持中文
    ss << "\\usepackage[a4paper, margin=1in]{geometry}\n"; // 设置页面边距
    ss << "\\usepackage{titlesec}\n\n";
    ss << "% 自定义节标题格式\n";
    ss << "\\titleformat{\\section}{\\Large\\bfseries}{\\thesection}{1em}{}\n";
    ss << "\\titleformat{\\subsection}{\\large\\bfseries}{\\thesubsection}{1em}{}\n\n";
    
    ss << "\\title{" << data.year << "年" << data.month << "月 消费报告}\n";
    ss << "\\author{BillsMaster}\n";
    ss << "\\date{\\today}\n\n";

    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";

    // 2. 总体摘要
    ss << "\\begin{abstract}\n";
    ss << "    \\textbf{总支出：} ¥" << data.grand_total << "\\\\\n";
    ss << "    \\textbf{备注：} " << escape_latex(data.remark) << "\n";
    ss << "\\end{abstract}\n\n";

    // 3. 遍历所有类别并生成内容
    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;

        ss << "\\section*{" << escape_latex(parent_name) << "}\n";
        ss << "总计：¥" << parent_data.parent_total 
           << " \t (占总支出: " << parent_percentage << "\\%)\n\n";
        
        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            ss << "\\subsection*{" << escape_latex(sub_name) << "}\n";
            ss << "\\textbf{小计：} ¥" << sub_data.sub_total 
               << " (占该类: " << sub_percentage << "\\%)\n";
            
            ss << "\\begin{itemize}\n";
            for (const auto& t : sub_data.transactions) {
                ss << "    \\item ¥" << t.amount << " --- " << escape_latex(t.description) << "\n";
            }
            ss << "\\end{itemize}\n\n";
        }
        ss << "\\hrule\n\n"; // 添加分隔线
    }

    ss << "\\end{document}\n";

    return ss.str();
}