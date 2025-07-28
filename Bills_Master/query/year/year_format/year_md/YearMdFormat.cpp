// YearMdFormat.cpp
#include "YearMdFormat.h"
#include <sstream>
#include <iomanip>

// 构造函数实现:初始化配置成员
YearMdFormat::YearMdFormat(const YearMdConfig& config) : config(config) {}

std::string YearMdFormat::format_report(const YearlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2;
        return ss.str();
    }

    ss << std::fixed << std::setprecision(config.precision);

    // --- 年度总计标题:硬编码 '##', ':**' 和 '**'，使用配置项组合标签 ---
    ss << "\n## " << data.year << config.yearly_total_label << ":**" 
       << data.grand_total << " " << config.currency_name << "**\n";

    // --- 每月支出标题:硬编码 '## ' 前缀 ---
    ss << "\n## " << config.monthly_breakdown_title << "\n";

    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        
        // --- 每月条目:硬编码 ' - ' 前缀，并使用通用的 currency_name ---
        ss << " - " << data.year << config.monthly_item_date_separator
           << std::setfill(config.fill_char) << std::setw(config.month_width) << month_val 
           << config.monthly_item_value_separator << month_total << " " << config.currency_name << "\n";
    }

    return ss.str();
}

// =================================================================
// ================== 以下是为动态库添加的部分 ==================
// =================================================================
extern "C" {

    // 定义平台特定的导出宏
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    /**
     * @brief 创建 YearMdFormat 格式化器实例的工厂函数。
     * @return 指向 IYearlyReportFormatter 接口的指针。
     *
     * 这是此动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象。
     */
    PLUGIN_API IYearlyReportFormatter* create_md_year_formatter() {
        // 创建并返回一个新的格式化器实例
        return new YearMdFormat();
    }

} // extern "C"