// reports/monthly_report/ReportSorter.cpp

// ReportSorter.cpp
#include "report_sorter.hpp"

#include <algorithm>  // for std::sort

// 实现静态方法
auto ReportSorter::sort_report_data(const MonthlyReportData& data)
    -> std::vector<std::pair<std::string, ParentCategoryData>> {
  // 1. 内部交易按金额排序
  auto sorted_data = data.aggregated_data;  // 创建一个副本以进行排序
  for (auto& parent_pair : sorted_data) {
    for (auto& sub_pair : parent_pair.second.sub_categories) {
      std::ranges::sort(sub_pair.second.transactions,
                        [](const Transaction& left,
                           const Transaction& right) -> bool {
                          return left.amount > right.amount;
                        });
    }
  }

  // 2. 父类别按总金额排序
  std::vector<std::pair<std::string, ParentCategoryData>> sorted_parents;
  for (const auto& pair : sorted_data) {
    sorted_parents.emplace_back(pair);
  }
  std::ranges::sort(sorted_parents, [](const auto& left, const auto& right)
                                        -> auto {
    return left.second.parent_total > right.second.parent_total;
  });

  return sorted_parents;
}
