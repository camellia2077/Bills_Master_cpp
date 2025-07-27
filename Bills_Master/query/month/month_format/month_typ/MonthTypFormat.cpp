// MonthTypFormat.cpp
#include "MonthTypFormat.h"
#include "query/month/common/ReportSorter.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

// 构造函数保持不变
MonthTypFormat::MonthTypFormat(const MonthTypConfig& config) : config_(config) {}

// escape_typst 函数保持不变
std::string MonthTypFormat::escape_typst(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    for (const char c : input) {
        switch (c) {
            case '\\': output += "\\\\"; break;
            case '*':  output += "\\*";  break;
            case '_':  output += "\\_";  break;
            case '#':  output += "\\#";  break;
            default:   output += c;    break;
        }
    }
    return output;
}

std::string MonthTypFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    // --- 文档头部设置 (使用新的日期连接符) ---
    ss << "#set document(title: \"" << data.year << config_.labels.year_suffix << data.month << config_.labels.month_suffix << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";

    // --- 报告内容主体 (使用新的日期连接符) ---
    ss << "= " << data.year << config_.labels.year_suffix << data.month << config_.labels.month_suffix << config_.labels.report_title_suffix << "\n\n";

    if (!data.data_found) {
        ss << config_.labels.no_data_found << "\n";
        return ss.str();
    }

    auto sorted_parents = ReportSorter::sort_report_data(data);
    
    ss << std::fixed << std::setprecision(config_.decimal_precision);

    // 摘要部分 (完全来自 config)
    ss << "*" << config_.labels.grand_total << ":* " << config_.currency_symbol << data.grand_total << "\n";
    ss << "*" << config_.labels.remark << ":* " << escape_typst(data.remark) << "\n\n";

    // 遍历所有类别并使用配置生成内容
    for (const auto& parent_pair : sorted_parents) {
        const auto& parent_name = parent_pair.first;
        const auto& parent_data = parent_pair.second;
        double parent_percentage = (data.grand_total > 0) ? (parent_data.parent_total / data.grand_total) * 100.0 : 0.0;

        ss << "== " << escape_typst(parent_name) << "\n\n";
        ss << "*" << config_.labels.category_total << ":* " << config_.currency_symbol << parent_data.parent_total << "\n";
        ss << "*" << config_.labels.percentage_share << ":* " << parent_percentage << config_.percentage_symbol << "\n\n";

        for (const auto& sub_pair : parent_data.sub_categories) {
            const auto& sub_name = sub_pair.first;
            const auto& sub_data = sub_pair.second;
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;

            ss << "=== " << escape_typst(sub_name) << "\n";
            ss << "  *" << config_.labels.sub_category_total << ":* " << config_.currency_symbol << sub_data.sub_total 
               << " (" << config_.labels.percentage_share << ": " << sub_percentage << config_.percentage_symbol << ")\n";

            for (const auto& t : sub_data.transactions) {
                ss << "  - " << config_.currency_symbol << t.amount << " " << escape_typst(t.description) << "\n";
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
     * @brief 创建 MonthTypFormat 格式化器实例的工厂函数。
     * @return 指向 IMonthReportFormatter 接口的指针。
     *
     * 这是动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象，而无需知道具体的实现类。
     */
    PLUGIN_API IMonthReportFormatter* create_formatter() {
        // 创建并返回一个新的格式化器实例
        return new MonthTypFormat();
    }
    
} // extern "C"