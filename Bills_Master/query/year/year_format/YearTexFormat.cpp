#include "YearTexFormat.h"
#include <sstream>
#include <iomanip>

std::string YearTexFormat::format_report(const YearlyReportData& data) const{
    std::stringstream ss;

    // --- 文档前导和标题 ---
    ss << "\\documentclass[12pt]{article}\n";
    ss << "\\usepackage{ctex}\n";
    ss << "\\usepackage[a4paper, margin=1in]{geometry}\n";
    ss << "\\title{" << data.year << "年 消费总览}\n";
    ss << "\\author{BillsMaster}\n";
    ss << "\\date{\\today}\n\n";
    ss << "\\begin{document}\n";
    ss << "\\maketitle\n\n";

    if (!data.data_found) {
        ss << "未找到 " << data.year << " 年的任何数据。\n";
        ss << "\\end{document}\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);

    // --- 摘要部分 ---
    ss << "\\section*{年度总览}\n";
    ss << "\\begin{itemize}\n";
    ss << "    \\item \\textbf{年度总支出：} ¥" << data.grand_total << "\n";
    ss << "\\end{itemize}\n\n";

    // --- 每月支出详情 ---
    ss << "\\section*{每月支出}\n";
    ss << "\\begin{itemize}\n";
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "    \\item " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << "：¥" << month_total << "\n";
    }
    ss << "\\end{itemize}\n\n";

    ss << "\\end{document}\n";

    return ss.str();
}