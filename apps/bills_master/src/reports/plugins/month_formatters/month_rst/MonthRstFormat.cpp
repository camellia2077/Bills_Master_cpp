// reports/plugins/month_formatters/month_rst/MonthRstFormat.cpp

// MonthRstFormat.cpp
#include "MonthRstFormat.hpp"
#include "reports/components/monthly_report/ReportSorter.hpp" // 排序器头文件
#include <sstream>
#include <iomanip>
#include <vector>

// 构造函数实现:初始化配置成员
MonthRstFormat::MonthRstFormat(const MonthRstConfig& config) : config(config) {}

// format_report 方法现在组合硬编码的RST语法和来自配置的标签
std::string MonthRstFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2 
           << data.month << config.not_found_msg_part3;
        return ss.str();
    }
    
    auto sorted_parents = ReportSorter::sort_report_data(data);

    ss << std::fixed << std::setprecision(config.precision);
    
    // --- 标题 ---
    std::string title = std::to_string(data.year) + "年" + std::to_string(data.month) + config.report_title_suffix;
    ss << title << "\n";
    ss << std::string(title.length() * 2, config.title_char) << "\n\n";

    // --- 元数据 ---
    ss << config.total_prefix << config.currency_symbol << data.grand_total << "\n";
    ss << config.remark_prefix << data.remark << "\n\n";

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;

        // --- 父分类 ---
        ss << parent_name << "\n";
        ss << std::string(parent_name.length() * 2, config.parent_char) << "\n";
        
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        ss << config.parent_total_label << config.currency_symbol << parent_data.parent_total << "\n";
        ss << config.parent_percentage_label << parent_percentage << config.percentage_symbol << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;
            
            // --- 子分类 ---
            ss << sub_name << "\n";
            ss << std::string(sub_name.length() * 2, config.sub_char) << "\n";

            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << config.sub_total_label << config.currency_symbol << sub_data.sub_total 
               << config.sub_percentage_label_prefix << sub_percentage << config.sub_percentage_label_suffix << "\n\n";

            // --- 交易项 ---
            for (const auto& t : sub_data.transactions) {
                ss << config.transaction_char << " " << config.currency_symbol << t.amount << " " << t.description << "\n";
            }
            ss << "\n";
        }
    }

    return ss.str();
}

extern "C" {

    // 定义平台特定的导出宏
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif
    
    /**
     * @brief 创建 MonthRstFormat 格式化器实例的工厂函数。
     * @return 指向 IMonthReportFormatter 接口的指针。
     *
     * 这是动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象，而无需知道具体的实现类。
     */
    

    PLUGIN_API IMonthReportFormatter* create_rst_month_formatter() {
        // 创建并返回一个新的格式化器实例
        return new MonthRstFormat();
    }
    
} // extern "C"