// MonthMdFormat.cpp
#include "MonthMdFormat.h"
#include "query/month/common/ReportSorter.h" // 排序器头文件
#include <sstream>
#include <iomanip>
#include <vector>

// 构造函数实现:初始化配置成员
MonthMdFormat::MonthMdFormat(const MonthMdConfig& config) : config(config) {}

// format_report 方法现在组合硬编码的Markdown语法和来自配置的标签
std::string MonthMdFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2 
           << data.month << config.not_found_msg_part3;
        return ss.str();
    }

    auto sorted_parents = ReportSorter::sort_report_data(data);

    ss << std::fixed << std::setprecision(config.precision);

    // --- 元数据:硬编码'# '前缀 ---
    ss << "\n# " << config.date_label << data.year << std::setfill(config.fill_char) << std::setw(config.month_width) << data.month << std::endl;
    ss << "# " << config.total_label << config.currency_symbol << data.grand_total << std::endl;
    ss << "# " << config.remark_label << data.remark << std::endl;

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;
        
        // --- 父分类:硬编码'# '前缀 ---
        ss << "\n# " << parent_name << std::endl;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;
        ss << config.parent_total_label << config.currency_symbol << parent_data.parent_total << std::endl;
        ss << config.parent_percentage_label << parent_percentage << config.percentage_symbol << std::endl;

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;
            
            // --- 子分类:硬编码'## '前缀 ---
            ss << "\n## " << sub_name << std::endl;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << config.sub_total_label << config.currency_symbol << sub_data.sub_total 
               << config.sub_percentage_label_prefix << sub_percentage << config.sub_percentage_label_suffix << std::endl;

            for (const auto& t : sub_data.transactions) {
                // --- 交易项:硬编码'- '前缀 ---
                ss << "- " << config.currency_symbol << t.amount << " " << t.description << std::endl;
            }
        }
    }

    return ss.str();
}
extern "C" {

    // 定义平台特定的导出宏
    // - 在Windows上，__declspec(dllexport) 会将函数导出到DLL。
    // - 在Linux/macOS上，__attribute__((visibility("default"))) 实现同样的效果。
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif
    
    /**
     * @brief 创建 MonthMdFormat 格式化器实例的工厂函数。
     * @return 指向 IMonthReportFormatter 接口的指针。
     *
     * 这是动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象，而无需知道具体的实现类。
     */
    PLUGIN_API IMonthReportFormatter* create_formatter() {
        // 创建并返回一个新的格式化器实例
        return new MonthMdFormat();
    }
    
    } // extern "C"