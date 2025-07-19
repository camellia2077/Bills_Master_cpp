// MonthRstFormat.cpp
#include "MonthRstFormat.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

std::string MonthRstFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        return ss.str();
    }
    // --- 排序逻辑 (和其它格式化器完全一样) ---
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

    // --- 构建 reStructuredText (RST) 格式的报告字符串 ---
    ss << std::fixed << std::setprecision(2);
    
    // RST 的一级标题
    std::string title = std::to_string(data.year) + "年" + std::to_string(data.month) + "月 消费报告";
    ss << title << "\n";
    ss << std::string(title.length() * 2, '=') << "\n\n"; // RST 标题下划线

    // 摘要信息
    ss << "**总支出:** ¥" << data.grand_total << "\n";
    ss << "**备注:** " << data.remark << "\n\n";

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        // RST 的二级标题
        ss << parent_name << "\n";
        ss << std::string(parent_name.length() * 2, '-') << "\n";
        
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        ss << "总计:¥" << parent_data.parent_total << "\n";
        ss << "占比:" << parent_percentage << "%" << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;
            
            // RST 的三级标题
            ss << sub_name << "\n";
            ss << std::string(sub_name.length() * 2, '^') << "\n";

            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << "小计:¥" << sub_data.sub_total << "(占比:" << sub_percentage << "%)" << "\n\n";

            // RST 的无序列表
            for (const auto& t : sub_data.transactions) {
                ss << "* ¥" << t.amount << " " << t.description << "\n";
            }
            ss << "\n";
        }
    }

    return ss.str();
}