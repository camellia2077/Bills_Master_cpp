// MonthRstFormat.cpp
#include "MonthRstFormat.h"
#include "query/month/common/ReportSorter.h" // 排序器头文件
#include <sstream>
#include <iomanip>
#include <vector>
// #include <algorithm> // 同样可以移除

std::string MonthRstFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << "未找到 " << data.year << "年" << data.month << "月的任何数据。\n";
        return ss.str();
    }
    
    // --- 排序 ---
    // 2. 直接调用 ReportSorter 来获取排好序的数据
    auto sorted_parents = ReportSorter::sort_report_data(data);

    // --- 构建 reStructuredText (RST) 格式的报告字符串 ---
    // (这部分代码也完全不需要改变)
    ss << std::fixed << std::setprecision(2);
    
    std::string title = std::to_string(data.year) + "年" + std::to_string(data.month) + "月 消费报告";
    ss << title << "\n";
    ss << std::string(title.length() * 2, '=') << "\n\n";

    ss << "**总支出:** ¥" << data.grand_total << "\n";
    ss << "**备注:** " << data.remark << "\n\n";

    for (const auto& parent_pair : sorted_parents) {
        // ... (后续渲染代码保持不变)
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        ss << parent_name << "\n";
        ss << std::string(parent_name.length() * 2, '-') << "\n";
        
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        ss << "总计:¥" << parent_data.parent_total << "\n";
        ss << "占比:" << parent_percentage << "%" << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;
            
            ss << sub_name << "\n";
            ss << std::string(sub_name.length() * 2, '^') << "\n";

            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << "小计:¥" << sub_data.sub_total << "(占比:" << sub_percentage << "%)" << "\n\n";

            for (const auto& t : sub_data.transactions) {
                ss << "* ¥" << t.amount << " " << t.description << "\n";
            }
            ss << "\n";
        }
    }

    return ss.str();
}