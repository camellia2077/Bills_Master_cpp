
// ReportSorter.cpp
#include "ReportSorter.hpp"
#include <algorithm> // for std::sort

// 实现静态方法
std::vector<std::pair<std::string, ParentCategoryData>> ReportSorter::sort_report_data(const MonthlyReportData& data) {
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

    return sorted_parents;
}