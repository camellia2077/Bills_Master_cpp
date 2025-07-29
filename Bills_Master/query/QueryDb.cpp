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
      m_export_base_dir(export_base_dir),
      m_format_folder_names(format_folder_names),
      m_month_manager(plugin_directory_path), //
      m_year_manager(plugin_directory_path)    //
{
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
    }
}

QueryFacade::QueryFacade(const std::string& db_path, const std::vector<std::string>& plugin_paths, const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_db(nullptr), 
      m_export_base_dir(export_base_dir),
      m_format_folder_names(format_folder_names),
      m_month_manager(plugin_paths), //
      m_year_manager(plugin_paths)    //
{
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
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
    YearlyDataReader reader(m_db);
    YearlyReportData data = reader.read_yearly_data(year);
    auto formatter = m_year_manager.createFormatter(format_name);
    if (!formatter) {
        throw std::runtime_error("Yearly formatter for '" + format_name + "' is not available or failed to load.");
    }
    return formatter->format_report(data);
}


std::string QueryFacade::get_monthly_details_report(int year, int month, const std::string& format_name) {
    MonthQuery reader(m_db);
    MonthlyReportData data = reader.read_monthly_data(year, month);
    auto formatter = m_month_manager.createFormatter(format_name);
    if (!formatter) {
        throw std::runtime_error("Monthly formatter for '" + format_name + "' is not available or failed to load.");
    }
    return formatter->format_report(data);
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
    // --- 新增：在执行任何操作前，先检查所需插件是否都已成功加载 ---
    const bool month_plugin_loaded = m_month_manager.isFormatAvailable(format_name);
    const bool year_plugin_loaded = m_year_manager.isFormatAvailable(format_name);

    // 如果月度或年度插件有任何一个未加载，则中止导出
    if (!month_plugin_loaded || !year_plugin_loaded) {
        // 打印缺失的dll
        std::cerr << "\n" << RED_COLOR << "Export Aborted:" << RESET_COLOR
                  << " The required format plugin '" << format_name << "' could not be loaded." << std::endl;
        
        // 分别给出具体的缺失插件提示
        if (!month_plugin_loaded) {
             std::cerr << " -> Please ensure the monthly formatter plugin (e.g., '" 
                       << format_name << "_month_formatter.dll') is available and correct." << std::endl;
        }
        if (!year_plugin_loaded) {
             std::cerr << " -> Please ensure the yearly formatter plugin (e.g., '" 
                       << format_name << "_year_formatter.dll') is available and correct." << std::endl;
        }
        return false; // 中止操作
    }
    // --- 检查结束 ---

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
                // 在抑制模式下，打印简要错误
                std::cerr << RED_COLOR << " FAILED" << RESET_COLOR << " (see details above if any)";
                monthly_stats.failure++;
                overall_success = false;
            }
        }

        std::cout << "\n\n--- Exporting Yearly Reports ---\n";
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
                 // 在抑制模式下，打印简要错误
                std::cerr << RED_COLOR << " FAILED" << RESET_COLOR << " (see details above if any)";
                yearly_stats.failure++;
                overall_success = false;
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "\nAn unexpected error occurred during export: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure++; //
        overall_success = false;
    }

    std::cout << "\n";
    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    
    if(overall_success) {
        std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
    } else {
        std::cout << "\n" << RED_COLOR << "Failed: " << RESET_COLOR << "Full report export completed with errors.\n";
    }

    return overall_success;
}