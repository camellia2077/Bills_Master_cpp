#include "QueryDb.h"
#include "query/year/YearlyReportGenerator.h"
#include "query/month/MonthlyReportGenerator.h"
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
QueryFacade::QueryFacade(const std::string& db_path) : m_db(nullptr) {
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

// --- 报告生成方法 ---

std::string QueryFacade::get_yearly_summary_report(int year, const std::string& format_name) {
    // =================================================================================
    // == 临时兼容性层 (技术债) START
    // == 因为 YearlyReportGenerator 尚未更新为插件模型，我们在这里手动将字符串转回旧的枚举
    // == 当 YearlyReportGenerator 也被修改后，应移除此部分，并直接传递 format_name
    // =================================================================================
    ReportFormat format_enum = ReportFormat::Markdown; // 默认值
    static const std::map<std::string, ReportFormat> format_map = {
        {"md", ReportFormat::Markdown},
        {"tex", ReportFormat::LaTeX},
        {"typ", ReportFormat::Typst},
        {"rst", ReportFormat::Rst}
    };
    auto it = format_map.find(format_name);
    if (it != format_map.end()) {
        format_enum = it->second;
    } else {
        // 如果格式不被年报生成器支持，可以抛出异常或使用默认值
        // 这里我们选择使用默认的 Markdown
    }
    
    YearlyReportGenerator generator(m_db);
    return generator.generate(year, format_enum); // 调用旧的、基于枚举的接口
    // =================================================================================
    // == 临时兼容性层 END
    // =================================================================================
}

std::string QueryFacade::get_monthly_details_report(int year, int month, const std::string& format_name) {
    MonthlyReportGenerator generator(m_db);
    // 直接将 format_name 字符串传递给新的、基于插件的生成器
    return generator.generate(year, month, format_name);
}

// --- 数据查询方法 (无变化) ---
std::vector<std::string> QueryFacade::get_all_bill_dates() {
    std::vector<std::string> dates;
    const char* sql = "SELECT DISTINCT year, month FROM transactions ORDER BY year, month;";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare SQL statement to get all dates: " + std::string(sqlite3_errmsg(m_db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int year = sqlite3_column_int(stmt, 0);
        int month = sqlite3_column_int(stmt, 1);
        
        std::stringstream ss;
        ss << year << std::setfill('0') << std::setw(2) << month;
        dates.push_back(ss.str());
    }

    sqlite3_finalize(stmt);
    return dates;
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

void QueryFacade::export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output) {
    try {
        if (month_str.length() != 6) throw std::invalid_argument("Invalid month format.");
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));
        std::string report = get_monthly_details_report(year, month, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;
            // **修改点**: 调用辅助函数来获取统一的目录名
            std::string display_format = get_display_format_name(format_name);
            std::string base_dir = "exported_files/" + display_format + "_bills";

            fs::path target_dir = fs::path(base_dir) / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
    }
}

void QueryFacade::export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = get_yearly_summary_report(year, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;
            // **修改点**: 移除旧的switch/if-else，调用辅助函数来获取统一的目录名
            std::string display_format = get_display_format_name(format_name);
            std::string base_dir = "exported_files/" + display_format + "_bills";

            fs::path target_dir = fs::path(base_dir) / "years";
            fs::path output_path = target_dir / (year_str + extension);
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
    }
}

void QueryFacade::export_all_reports(const std::string& format_name) {
    ProcessStats monthly_stats, yearly_stats;
    
    // **修改点**: 直接使用 format_name 字符串，移除了 switch
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";

    try {
        std::vector<std::string> all_months = get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found in the database. Nothing to export.\n";
            return;
        }

        std::cout << "Found " << all_months.size() << " unique months to process.\n";
        
        std::cout << "\n--- Exporting Monthly Reports ---\n";
        for (const auto& month : all_months) {
            std::cout << "Exporting report for " << month << "...";
            export_monthly_report(month, format_name, true);
            std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            monthly_stats.success++;
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
            export_yearly_report(year, format_name, true);
            std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            yearly_stats.success++;
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1;
    }

    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
}