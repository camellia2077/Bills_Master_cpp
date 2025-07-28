#include "QueryDb.h"
#include "query/year/YearlyReportGenerator.h"
#include "query/month/MonthlyReportGenerator.h"
#include "query/metadata_reader/BillMetadataReader.h" // 获取所有日期
#include "app_controller/ProcessStats.h"
#include "common/common_utils.h"
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <map> // 用于临时的字符串到枚举映射

namespace fs = std::filesystem;

// 构造函数和析构函数保持不变
QueryFacade::QueryFacade(const std::string& db_path, const std::vector<std::string>& plugin_paths)
    : m_db(nullptr), m_plugin_paths(plugin_paths), m_use_plugin_list(true) {
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("Cannot open database: " + errmsg);
    }
}

QueryFacade::~QueryFacade() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}
std::vector<std::string> QueryFacade::get_all_bill_dates() {
    BillMetadataReader reader(m_db);
    return reader.get_all_bill_dates();
}
// --- 报告生成方法 ---

std::string QueryFacade::get_yearly_summary_report(int year, const std::string& format_name) {
    // 根据标志位选择不同的生成器初始化方式
    if (m_use_plugin_list) {
        YearlyReportGenerator generator(m_db, m_plugin_paths);
        return generator.generate(year, format_name);
    } else {
        YearlyReportGenerator generator(m_db, m_plugin_directory_path);
        return generator.generate(year, format_name);
    }
}
std::string QueryFacade::get_monthly_details_report(int year, int month, const std::string& format_name) {
    // 根据标志位选择不同的生成器初始化方式
    if (m_use_plugin_list) {
        MonthlyReportGenerator generator(m_db, m_plugin_paths);
        return generator.generate(year, month, format_name);
    } else {
        MonthlyReportGenerator generator(m_db, m_plugin_directory_path);
        return generator.generate(year, month, format_name);
    }
}



std::string QueryFacade::get_display_format_name(const std::string& short_name) const {
    if (short_name == "md") return "Markdown";
    if (short_name == "tex") return "LaTeX";
    if (short_name == "rst") return "reST";
    if (short_name == "typ") return "typst"; // 根据工具名，typst可能比typ更合适
    return short_name; // 如果有新格式，先用短名称作为回退
}



// --- 报告导出实现 ---

void QueryFacade::save_report(const std::string& report_content, const std::string& file_path_str) {
    fs::path file_path(file_path_str);
    fs::create_directories(file_path.parent_path());

    std::ofstream output_file(file_path);
    if (!output_file) {
        throw std::runtime_error("Could not open file for writing: " + file_path.string());
    }
    output_file << report_content;
}

bool QueryFacade::export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output) {
    try {
        if (month_str.length() != 6) throw std::invalid_argument("Invalid month format.");
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));
        std::string report = get_monthly_details_report(year, month, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;
            std::string display_format = get_display_format_name(format_name);
            std::string base_dir = "exported_files/" + display_format + "_bills";

            fs::path target_dir = fs::path(base_dir) / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true; // 操作成功
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false; // 操作失败
    }
}


bool QueryFacade::export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = get_yearly_summary_report(year, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;
            std::string display_format = get_display_format_name(format_name);
            std::string base_dir = "exported_files/" + display_format + "_bills";

            fs::path target_dir = fs::path(base_dir) / "years";
            fs::path output_path = target_dir / (year_str + extension);
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true; // 操作成功
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false; // 操作失败
    }
}

bool QueryFacade::export_all_reports(const std::string& format_name) {
    ProcessStats monthly_stats, yearly_stats;
    bool overall_success = true; // 追踪整体操作是否成功
    
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";

    try {
        // 使用新的 BillMetadataReader 来获取日期
        BillMetadataReader metadata_reader(m_db);
        std::vector<std::string> all_months = metadata_reader.get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found in the database. Nothing to export.\n";
            return true; // 没有数据可导出，但操作本身是成功的
        }

        std::cout << "Found " << all_months.size() << " unique months to process.\n";
        
        std::cout << "\n--- Exporting Monthly Reports ---\n";
        for (const auto& month : all_months) {
            std::cout << "Exporting report for " << month << "...";
            if (export_monthly_report(month, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                monthly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                monthly_stats.failure++;
                overall_success = false; // 标记为失败
            }
        }

        std::cout << "\n--- Exporting Yearly Reports ---\n";
        std::set<std::string> unique_years;
        for (const auto& month : all_months) {
            if (month.length() >= 4) {
                unique_years.insert(month.substr(0, 4));
            }
        }
        for (const auto& year : unique_years) {
            std::cout << "Exporting summary for " << year << "...";
            if (export_yearly_report(year, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                yearly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                yearly_stats.failure++;
                overall_success = false; // 标记为失败
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1;
        overall_success = false; // 标记为失败
    }

    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    
    if(overall_success) {
        std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
    } else {
        std::cout << "\n" << RED_COLOR << "Failed: " << RESET_COLOR << "Full report export completed with errors.\n";
    }

    return overall_success;
}