#include "QueryDb.h"
#include "query/year/YearlyReportGenerator.h"
#include "query/month/MonthlyReportGenerator.h"
#include "query/metadata_reader/BillMetadataReader.h"
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
#include <map>

namespace fs = std::filesystem;

// --- 修改: 构造函数实现已更新，以初始化新成员 ---
QueryFacade::QueryFacade(const std::string& db_path, const std::string& plugin_directory_path, const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_db(nullptr), 
      m_plugin_directory_path(plugin_directory_path), 
      m_use_plugin_list(false), 
      m_export_base_dir(export_base_dir),
      m_format_folder_names(format_folder_names) {
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw std::runtime_error("Cannot open database: " + errmsg);
    }
}

QueryFacade::QueryFacade(const std::string& db_path, const std::vector<std::string>& plugin_paths, const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_db(nullptr), 
      m_plugin_paths(plugin_paths), 
      m_use_plugin_list(true),
      m_export_base_dir(export_base_dir),
      m_format_folder_names(format_folder_names) {
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
    if (m_use_plugin_list) {
        YearlyReportGenerator generator(m_db, m_plugin_paths);
        return generator.generate(year, format_name);
    } else {
        YearlyReportGenerator generator(m_db, m_plugin_directory_path);
        return generator.generate(year, format_name);
    }
}
std::string QueryFacade::get_monthly_details_report(int year, int month, const std::string& format_name) {
    if (m_use_plugin_list) {
        MonthlyReportGenerator generator(m_db, m_plugin_paths);
        return generator.generate(year, month, format_name);
    } else {
        MonthlyReportGenerator generator(m_db, m_plugin_directory_path);
        return generator.generate(year, month, format_name);
    }
}

// 格式名称映射
std::string QueryFacade::get_display_format_name(const std::string& short_name) const {
    if (short_name == "md") return "Markdown";
    if (short_name == "tex") return "LaTeX";
    if (short_name == "rst") return "reST";
    if (short_name == "typ") return "typst";
    return short_name;
}

// --- 报告导出实现 (无变化) ---
void QueryFacade::save_report(const std::string& report_content, const std::string& file_path_str) {
    fs::path file_path(file_path_str);
    fs::create_directories(file_path.parent_path());
    std::ofstream output_file(file_path);
    if (!output_file) {
        throw std::runtime_error("Could not open file for writing: " + file_path.string());
    }
    output_file << report_content;
}

// --- 修改: 导出逻辑使用新的配置变量 ---
bool QueryFacade::export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output) {
    try {
        if (month_str.length() != 6) throw std::invalid_argument("Invalid month format.");
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));
        std::string report = get_monthly_details_report(year, month, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;
            
            // 动态确定格式文件夹名称
            std::string format_folder;
            auto it = m_format_folder_names.find(format_name);
            if (it != m_format_folder_names.end()) {
                format_folder = it->second; // 使用用户提供的名称
            } else {
                // 回退到默认命名规则
                format_folder = get_display_format_name(format_name) + "_bills";
            }
            
            // 使用 fs::path 构建路径，更安全可靠
            fs::path base_dir = fs::path(m_export_base_dir) / format_folder;
            fs::path target_dir = base_dir / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}

// --- 修改: 导出逻辑使用新的配置变量 ---
bool QueryFacade::export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = get_yearly_summary_report(year, format_name);

        if (!suppress_output) std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            std::string extension = "." + format_name;

            // 动态确定格式文件夹名称
            std::string format_folder;
            auto it = m_format_folder_names.find(format_name);
            if (it != m_format_folder_names.end()) {
                format_folder = it->second; // 使用用户提供的名称
            } else {
                // 回退到默认命名规则
                format_folder = get_display_format_name(format_name) + "_bills";
            }

            // 使用 fs::path 构建路径
            fs::path base_dir = fs::path(m_export_base_dir) / format_folder;
            fs::path target_dir = base_dir / "years";
            fs::path output_path = target_dir / (year_str + extension);
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}


bool QueryFacade::export_all_reports(const std::string& format_name) {
    ProcessStats monthly_stats, yearly_stats;
    bool overall_success = true; 
    
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";

    try {
        BillMetadataReader metadata_reader(m_db);
        std::vector<std::string> all_months = metadata_reader.get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found in the database. Nothing to export.\n";
            return true;
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
                overall_success = false;
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
                overall_success = false;
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1;
        overall_success = false;
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