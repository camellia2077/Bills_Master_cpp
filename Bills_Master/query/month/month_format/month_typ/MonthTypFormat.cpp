#include "MonthTypFormat.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>

// Typst 对特殊字符的处理比 LaTeX 友好得多，
// 但为了安全起见，我们还是转义一些可能引起冲突的字符。
std::string MonthTypFormat::escape_typst(const std::string& input) const { // <-- Add const here
    std::string output;
    output.reserve(input.size());
    for (const char c : input) {
        switch (c) {
            case '\\': output += "\\\\"; break;
            case '*':  output += "\\*";  break;
            case '_':  output += "\\_";  break;
            // Typst 中的 # 用于代码，最好也转义
            case '#':  output += "\\#";  break;
            default:   output += c;    break;
        }
    }
    return output;
}

std::string MonthTypFormat::format_report(const MonthlyReportData& data) const { // <-- Add const here
    std::stringstream ss;

    if (!data.data_found) {
        ss << "= " << data.year << "年" << data.month << "月消费报告\n\n";
        ss << "未找到该月的任何数据。\n";
        return ss.str();
    }

    // --- 排序逻辑 (与其他格式化器相同) ---
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

    // --- 构建 Typst 文档 ---
    ss << std::fixed << std::setprecision(2);

    // 1. 设置文档参数和标题
    ss << "#set document(title: \"" << data.year << "年" << data.month << "月消费报告\", author: \"BillsMaster\")\n";
    ss << "#set text(font: \"Noto Serif SC\", size: 12pt)\n\n";

    ss << "= " << data.year << "年" << data.month << "月 消费报告\n\n";

    // 2. 总体摘要
    ss << "*总支出:* ¥" << data.grand_total << "\n";
    ss << "*备注:* " << escape_typst(data.remark) << "\n\n";

    // 3. 遍历所有类别并生成内容
    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;

        // Typst 的标题
        ss << "== " << escape_typst(parent_name) << "\n\n";
        ss << "*总计:* ¥" << parent_data.parent_total << "\n";
        ss << "*占比:* " << parent_percentage << "%\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            // Typst 的子标题
            ss << "=== " << escape_typst(sub_name) << "\n";
            ss << "  *小计:* ¥" << sub_data.sub_total << " (占比: " << sub_percentage << "%)\n";

            // Typst 的列表
            for (const auto& t : sub_data.transactions) {
                ss << "  - ¥" << t.amount << " " << escape_typst(t.description) << "\n";
            }
            ss << "\n";
        }
    }

    return ss.str();
}