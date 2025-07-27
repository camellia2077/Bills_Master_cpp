#include "YearTypFormat.h"
#include <sstream>
#include <iomanip>

// 构造函数保持不变
YearTypFormat::YearTypFormat(const YearlyTypConfig& config) : config_(config) {}

std::string YearTypFormat::format_report(const YearlyReportData& data) const {
    std::stringstream ss;

    // --- 文档头部设置 (完全来自 config) ---
    ss << "#set document(title: \"" << data.year << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    
    // --- 报告内容主体 (完全来自 config) ---
    // 使用新的配置项替换硬编码的 "年 "
    ss << "= " << data.year << config_.labels.year_suffix << config_.labels.report_title_suffix << "\n\n";

    if (!data.data_found) {
        ss << config_.labels.no_data_found_prefix << data.year << config_.labels.no_data_found_suffix << "\n";
        return ss.str();
    }
    
    ss << std::fixed << std::setprecision(config_.decimal_precision);

    // --- 摘要部分 (完全来自 config) ---
    ss << "== " << config_.labels.overview_section_title << "\n\n";
    ss << "  - *" << config_.labels.grand_total << ":* " << config_.currency_symbol << data.grand_total << "\n\n";
    
    // --- 每月支出详情 (完全来自 config) ---
    ss << "== " << config_.labels.monthly_breakdown_section_title << "\n\n";
    for (const auto& pair : data.monthly_totals) {
        int month_val = pair.first;
        double month_total = pair.second;
        ss << "  - " << data.year << "-" << std::setfill('0') << std::setw(2) << month_val
           << ":" << config_.currency_symbol << month_total << "\n";
    }

    return ss.str();
}