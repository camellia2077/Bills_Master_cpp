// MonthMdFormat.cpp
#include "MonthMdFormat.h"
#include "query/month/common/ReportSorter.h" // 排序器头文件 
#include <sstream>
#include <iomanip>
#include <vector>
// #include <algorithm> // algorithm 现在可以移除了，因为排序在别处完成

std::string MonthMdFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "\n未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        return ss.str();
    }

    // --- 排序 ---
    // 2. 直接调用 ReportSorter 来获取排好序的数据
    auto sorted_parents = ReportSorter::sort_report_data(data);

    // --- 按最终格式构建报告字符串 ---
    // (这部分代码完全不需要改变)
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