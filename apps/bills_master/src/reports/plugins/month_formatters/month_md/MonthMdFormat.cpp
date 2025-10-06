// reports/plugins/month_formatters/month_md/MonthMdFormat.cpp

#include "MonthMdFormat.hpp"
#include "reports/components/monthly_report/ReportSorter.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath> // For std::abs

MonthMdFormat::MonthMdFormat(const MonthMdConfig& config) : config(config) {}

std::string MonthMdFormat::format_report(const MonthlyReportData& data) const {
    std::stringstream ss;

    if (!data.data_found) {
        ss << config.not_found_msg_part1 << data.year << config.not_found_msg_part2 
           << data.month << config.not_found_msg_part3;
        return ss.str();
    }

    auto sorted_parents = ReportSorter::sort_report_data(data);

    ss << std::fixed << std::setprecision(config.precision);

    // --- 【核心修改】: 更新元数据部分以显示新的总计 ---
    ss << "\n# " << config.date_label << data.year << std::setfill(config.fill_char) << std::setw(config.month_width) << data.month << std::endl;
    ss << "# " << config.income_label << config.currency_symbol << data.total_income << std::endl;
    ss << "# " << config.expense_label << config.currency_symbol << data.total_expense << std::endl;
    ss << "# " << config.balance_label << config.currency_symbol << data.balance << std::endl;
    ss << "# " << config.remark_label << data.remark << std::endl;
    // --- 修改结束 ---

    for (const auto& parent_pair : sorted_parents) {
        const std::string& parent_name = parent_pair.first;
        const ParentCategoryData& parent_data = parent_pair.second;
        
        ss << "\n# " << parent_name << std::endl;
        // --- 【核心修改】: 使用 total_expense 的绝对值作为百分比计算的基数 ---
        double parent_percentage = (data.total_expense != 0) ? (parent_data.parent_total / data.total_expense) * 100.0 : 0.0;
        ss << config.parent_total_label << config.currency_symbol << parent_data.parent_total << std::endl;
        ss << config.parent_percentage_label << std::abs(parent_percentage) << config.percentage_symbol << std::endl;
        // --- 修改结束 ---

        for (const auto& sub_pair : parent_data.sub_categories) {
            const std::string& sub_name = sub_pair.first;
            const SubCategoryData& sub_data = sub_pair.second;
            
            ss << "\n## " << sub_name << std::endl;
            double sub_percentage = (parent_data.parent_total != 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            ss << config.sub_total_label << config.currency_symbol << sub_data.sub_total 
               << config.sub_percentage_label_prefix << std::abs(sub_percentage) << config.sub_percentage_label_suffix << std::endl;

            for (const auto& t : sub_data.transactions) {
                ss << "- " << config.currency_symbol << t.amount << " " << t.description << std::endl;
            }
        }
    }

    return ss.str();
}

extern "C" {
    #ifdef _WIN32
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __attribute__((visibility("default")))
    #endif
    
    PLUGIN_API IMonthReportFormatter* create_md_month_formatter() {
        return new MonthMdFormat();
    }
}