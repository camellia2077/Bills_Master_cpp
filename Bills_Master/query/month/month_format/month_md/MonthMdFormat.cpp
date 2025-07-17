// MonthMdFormat.cpp
#include "MonthMdFormat.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

std::string MonthMdFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "\n未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        return ss.str();
    }

    // --- 排序 ---
    // 1. 内部交易按金额排序
    auto sorted_data = data.aggregated_data; // 创建一个副本以进行排序
    for (auto& parent_pair : sorted_data) {
        for (auto& sub_pair : parent_pair.second.sub_categories) {
            std::sort(sub_pair.second.transactions.begin(), sub_pair.second.transactions.end(),
                [](const Transaction& a, const Transaction& b) {
                    return a.amount > b.amount;
                });
        }
    }

    // 2. 父类别按总金额排序
    std::vector<std::pair<std::string, ParentCategoryData>> sorted_parents;
    for (const auto& pair : sorted_data) {
        sorted_parents.push_back(pair);
    }
    std::sort(sorted_parents.begin(), sorted_parents.end(),
        [](const auto& a, const auto& b) {
            return a.second.parent_total > b.second.parent_total;
        });

    // --- 按最终格式构建报告字符串 ---
    ss << std::fixed << std::setprecision(2);
    ss << "\n# DATE:" << data.year << std::setfill('0') << std::setw(2) << data.month << std::endl;
    ss << "# TOTAL:¥" << data.grand_total << std::endl;
    ss << "# REMARK:" << data.remark << std::endl;

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        ss << "\n# " << parent_name << std::endl;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        ss << "总计：¥" << parent_data.parent_total << std::endl;
        ss << "占比：" << parent_percentage << "%" << std::endl;

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;

            ss << "\n## " << sub_name << std::endl;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << "小计：¥" << sub_data.sub_total << "（占比：" << sub_percentage << "%）" << std::endl;

            for (const auto& t : sub_data.transactions) {
                ss << "- ¥" << t.amount << " " << t.description << std::endl;
            }
        }
    }

    return ss.str();
}