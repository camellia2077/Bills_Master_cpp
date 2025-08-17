#include "common/pch.h"
// YearRstFormat.cpp
#include "YearRstFormat.h"
#include <sstream>
#include <iomanip>

// This is the original implementation of the format_report method.
std::string YearRstFormat::format_report(const YearlyReportData& data) const{
    std::stringstream ss;

    // --- 标题 ---
    std::string title = std::to_string(data.year) + "年 消费总览";
    ss << title << "\n";
    // RST 标题的下划线，长度需要和标题一致
    ss << std::string(title.length() * 2, '=') << "\n\n"; 

    if (!data.data_found) {
        ss << "未找到 " << data.year << " 年的任何数据。\n";
        return ss.str();
    }

    ss << std::fixed << std::setprecision(2);

    // --- 摘要部分 ---
    ss << "**年度总支出:** CNY" << data.grand_total << "\n\n";
    
    // --- 每月支出详情 (二级标题) ---
    std::string subtitle = "每月支出";
    ss << subtitle << "\n";
    ss << std::string(subtitle.length() * 2, '-') << "\n\n";

    // 使用无序列表
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "* " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << ": CNY" << month_total << "\n";
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
     * @brief 创建 YearRstFormat 格式化器实例的工厂函数。
     * @return 指向 IYearlyReportFormatter 接口的指针。
     *
     * 这是此动态库的唯一入口点。主应用程序将加载此库并调用此函数
     * 来获取一个格式化器对象。
     */
    PLUGIN_API IYearlyReportFormatter* create_rst_year_formatter() {
        // 创建并返回一个新的格式化器实例
        return new YearRstFormat();
    }

} // extern "C"