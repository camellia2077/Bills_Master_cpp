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

std::string YearTypFormat::generate_monthly_breakdown_header() const {
    std::stringstream ss;
    // 1. 输出章节标题
    ss << "== " << config_.labels.monthly_breakdown_section_title << "\n\n";
    
    // 2. 开始 Typst 表格定义
    // columns: (auto, 1fr, 1fr, 1fr) 表示第一列自适应宽度，后三列平分剩余空间
    // inset: 10pt 设置单元格内边距
    // align: (x, y) => (left, center) 设置对齐方式
    // stroke: none / 1pt + gray 设置边框 (这里用简单的默认边框)
    ss << "#table(\n";
    ss << "  columns: (auto, 1fr, 1fr, 1fr),\n";
    ss << "  inset: 10pt,\n";
    ss << "  align: horizon,\n"; // 垂直居中
    
    // 3. 输出表头 (加粗强调)
    ss << "  [*" << config_.labels.table_header_month << "*], "
       << "[*" << config_.labels.table_header_income << "*], "
       << "[*" << config_.labels.table_header_expense << "*], "
       << "[*" << config_.labels.table_header_balance << "*],\n";
       
    return ss.str();
}


// --- 【核心修改 2】: 输出表格行数据 ---
std::string YearTypFormat::generate_monthly_item(int year, int month, const MonthlySummary& summary) const {
    std::stringstream ss;
    double balance = summary.income + summary.expense;
    
    ss << std::fixed << std::setprecision(config_.decimal_precision);
    
    // 输出一行数据的四个单元格
    // Typst 的 table 内容是连续的参数，所以这里只需按顺序输出 content block [] 即可
    ss << "  [" << year << "-" << std::setfill('0') << std::setw(2) << month << "], "
       << "[" << config_.currency_symbol << summary.income << "], "
       << "[" << config_.currency_symbol << summary.expense << "], "
       << "[" << config_.currency_symbol << balance << "],\n";
       
    return ss.str();
}


std::string YearTypFormat::generate_footer(const YearlyReportData& data) const {
    // 之前的 header 打开了 #table( ... ，这里需要一个右括号来结束它
    return ")\n";
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