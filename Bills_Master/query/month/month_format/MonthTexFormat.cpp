#include "MonthTexFormat.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>

// 实现 LaTeX 特殊字符转义
std::string MonthTexFormat::escape_latex(const std::string& input) {
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

std::string MonthTexFormat::format_report(const MonthlyReportData& data) {
    std::stringstream ss;

    // --- 1. 文档前导和字体设置 ---
    ss << "\\documentclass[12pt]{article}\n";
    ss << "\\usepackage[a4paper, margin=1in]{geometry}\n";
    ss << "\\usepackage{fontspec}\n";
    ss << "\\usepackage[nofonts]{ctex}\n\n";
    
    ss << "% --- Hardcoded Font Settings ---\n";
    ss << "\\setmainfont{Noto Serif SC}\n";
    ss << "\\setCJKmainfont{Noto Serif SC}\n\n";

    if (!data.data_found) {
        ss << "\\begin{document}\n";
        ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        ss << "\\end{document}\n";
        return ss.str();
    }

    // --- 排序逻辑 (保持不变) ---
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
    
    // 2. 文档信息
    ss << "\\usepackage{titlesec}\n";
    ss << "\\titleformat{\\section}{\\Large\\bfseries}{\\thesection}{1em}{}\n";
    ss << "\\titleformat{\\subsection}{\\large\\bfseries}{\\thesubsection}{1em}{}\n\n";
    
    ss << "\\title{" << data.year << "年" << data.month << "月 消费报告}\n";
    ss << "\\author{camellia}\n";
    ss << "\\date{\\today}\n\n";

    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";

    // --- 3. 手动创建的摘要部分 ---
    // *** 这是核心改动：用 \hrulefill 包裹摘要内容 ***
    ss << "% --- Manually created summary section with rules ---\n";
    ss << "\\vspace{1em} % 在顶部分割线上方增加一点空间\n";
    ss << "\\hrulefill\n"; // 顶部分割线
    ss << "\\begin{center}\n";
    ss << "    \\Large\\bfseries 摘要\\par\\vspace{1em} % 摘要标题\n";
    ss << "    \\large % 将后续文本设置为 large 尺寸\n";
    ss << "    \\textbf{总支出：} ¥" << data.grand_total << "\\\\\n"; // 用 \\ 换行
    ss << "    \\textbf{备注：} " << escape_latex(data.remark) << "\n";
    ss << "\\end{center}\n";
    ss << "\\hrulefill\n\n"; // 底部分割线

    // 4. 遍历所有类别并生成内容
    for (size_t i = 0; i < sorted_parents.size(); ++i) {
        const auto& parent_pair = sorted_parents[i];
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
            ss << "\\end{itemize}\n";
        }
    }

    ss << "\\end{document}\n";

    return ss.str();
}