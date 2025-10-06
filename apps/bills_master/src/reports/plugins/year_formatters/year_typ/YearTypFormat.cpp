// reports/plugins/year_formatters/year_typ/YearTypFormat.cpp

#include "YearTypFormat.hpp"
#include <sstream>
#include <iomanip>

YearTypFormat::YearTypFormat(const YearlyTypConfig& config) : config_(config) {}

// ... get_no_data_message 和 generate_header 函数保持不变 ...
std::string YearTypFormat::get_no_data_message(int year) const {
    std::stringstream ss;
    ss << config_.labels.no_data_found_prefix << year << config_.labels.no_data_found_suffix << "\n";
    return ss.str();
}

std::string YearTypFormat::generate_header(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << "#set document(title: \"" << data.year << config_.labels.report_title_suffix << "\", author: \"" << config_.author << "\")\n";
    ss << "#set text(font: \"" << config_.font_family << "\", size: " << static_cast<int>(config_.font_size_pt) << "pt)\n\n";
    ss << "= " << data.year << config_.labels.year_suffix << config_.labels.report_title_suffix << "\n\n";
    return ss.str();
}


// --- 【核心修改】: 更新摘要部分 ---
std::string YearTypFormat::generate_summary(const YearlyReportData& data) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    ss << "== " << config_.labels.overview_section_title << "\n\n";
    ss << "  - *" << config_.labels.yearly_income << ":* " << config_.currency_symbol << data.total_income << "\n";
    ss << "  - *" << config_.labels.yearly_expense << ":* " << config_.currency_symbol << data.total_expense << "\n";
    ss << "  - *" << config_.labels.yearly_balance << ":* " << config_.currency_symbol << data.balance << "\n\n";
    return ss.str();
}
// --- 修改结束 ---

// ... generate_monthly_breakdown_header 函数保持不变 ...
std::string YearTypFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    ss << "== " << config_.labels.monthly_breakdown_section_title << "\n\n";
    return ss.str();
}


// --- 【核心修改】: 更新月度项目 ---
std::string YearTypFormat::generate_monthly_item(int year, int month, const MonthlySummary& summary) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    ss << "  - " << year << "-" << std::setfill('0') << std::setw(2) << month
       << ": (" << config_.labels.monthly_income << ": " << config_.currency_symbol << summary.income 
       << ", " << config_.labels.monthly_expense << ": " << config_.currency_symbol << summary.expense << ")\n";
    return ss.str();
}
// --- 修改结束 ---

// ... generate_footer 和插件导出部分保持不变 ...
std::string YearTypFormat::generate_footer(const YearlyReportData& data) const {
    return "";
}

extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif

    PLUGIN_API IYearlyReportFormatter* create_typ_year_formatter() {
        return new YearTypFormat();
    }
}