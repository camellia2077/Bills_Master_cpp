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
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)) +
        "\nMake sure bills.sqlite3 and the exe file are in the same folder.");
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
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)) +
        "\nMake sure bills.sqlite3 and the exe file are in the same folder.");
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

// 导出独立的日期
bool QueryFacade::export_by_date(const std::string& date_str, const std::string& format_name) {
    // 情况1: 导出某一年份的所有月度报告 (例如 "2024") - 逻辑不变
    if (date_str.length() == 4) {
        std::cout << "\n--- Exporting all monthly reports for " << date_str << " (" << format_name << " format) ---\n";
        
        if (!m_month_manager.isFormatAvailable(format_name)) {
            std::cerr << RED_COLOR << "Error:" << RESET_COLOR << " Monthly formatter for '" << format_name << "' not loaded.\n";
            return false;
        }

        BillMetadataReader metadata_reader(m_db);
        std::vector<std::string> all_months = metadata_reader.get_all_bill_dates();
        std::vector<std::string> year_months;
        for (const auto& month : all_months) {
            if (month.rfind(date_str, 0) == 0) year_months.push_back(month);
        }

        if (year_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning:" << RESET_COLOR << " No data found for year " << date_str << ".\n";
            return true;
        }

        ProcessStats stats;
        std::cout << "Found " << year_months.size() << " months to export for year " << date_str << ".\n";
        for (const auto& month : year_months) {
            std::cout << "Exporting report for " << month << "...";
            // 这里调用 export_monthly_report 并抑制输出是正确的
            if (export_monthly_report(month, format_name, true)) {
                stats.success++;
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            } else {
                stats.failure++;
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
            }
        }
        stats.print_summary("Yearly Batch Export");
        return stats.failure == 0;
    } 
    // 情况2: 导出单个指定的月份报告 (例如 "202401")
    else if (date_str.length() == 6) {
        // 不再调用 export_monthly_report(..., false)，而是亲自处理逻辑
        try {
            int year = std::stoi(date_str.substr(0, 4));
            int month = std::stoi(date_str.substr(4, 2));
            // 1. 获取报告内容
            std::string report = get_monthly_details_report(year, month, format_name);

            // 2. 不打印报告内容到控制台

            // 3. 检查并保存文件
            if (report.find("未找到") == std::string::npos) {
                std::string extension = "." + format_name;
                std::string format_folder;
                auto it = m_format_folder_names.find(format_name);
                format_folder = (it != m_format_folder_names.end()) ? it->second : get_display_format_name(format_name) + "_bills";
                
                fs::path base_dir = fs::path(m_export_base_dir) / format_folder;
                fs::path target_dir = base_dir / "months" / date_str.substr(0, 4);
                fs::path output_path = target_dir / (date_str + extension);
                
                save_report(report, output_path.string());

                // 4. 始终打印成功保存的消息
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report saved to " << output_path.string() << "\n";
            } else {
                // 如果没找到数据，只打印报告的提示信息
                std::cout << report << std::endl;
            }
            return true;
        } catch (const std::exception& e) {
            std::cerr << RED_COLOR << "Export failed: " << RESET_COLOR << e.what() << std::endl;
            return false;
        }
    } 
    // 其他无效格式 - 逻辑不变
    else {
        std::cerr << RED_COLOR << "Error:" << RESET_COLOR << " Invalid date format for export: '" << date_str << "'. Please use YYYY or YYYYMM.\n";
        return false;
    }
}

// --- 新增：导出所有月度报告的实现 ---
bool QueryFacade::export_all_monthly_reports(const std::string& format_name) {
    if (!m_month_manager.isFormatAvailable(format_name)) {
        std::cerr << "\n" << RED_COLOR << "Export Aborted:" << RESET_COLOR 
                  << " Monthly formatter for '" << format_name << "' not loaded." << std::endl;
        return false;
    }

    ProcessStats monthly_stats;
    std::cout << "\n--- Starting Monthly Report Export (" << format_name << " format) ---\n";

    try {
        BillMetadataReader metadata_reader(m_db);
        std::vector<std::string> all_months = metadata_reader.get_all_bill_dates();
        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found." << std::endl;
            return true;
        }

        std::cout << "Found " << all_months.size() << " unique months to process.\n";
        for (const auto& month : all_months) {
            std::cout << "Exporting report for " << month << "...";
            if (export_monthly_report(month, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                monthly_stats.success++;
            } else {
                std::cerr << RED_COLOR << " FAILED\n" << RESET_COLOR;
                monthly_stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "\nAn unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }

    std::cout << "\n";
    monthly_stats.print_summary("Monthly Export");
    return monthly_stats.failure == 0;
}

// --- 新增：导出所有年度报告的实现 ---
bool QueryFacade::export_all_yearly_reports(const std::string& format_name) {
    if (!m_year_manager.isFormatAvailable(format_name)) {
        std::cerr << "\n" << RED_COLOR << "Export Aborted:" << RESET_COLOR 
                  << " Yearly formatter for '" << format_name << "' not loaded." << std::endl;
        return false;
    }

    ProcessStats yearly_stats;
    std::cout << "\n--- Starting Yearly Report Export (" << format_name << " format) ---\n";

    try {
        BillMetadataReader metadata_reader(m_db);
        std::vector<std::string> all_months = metadata_reader.get_all_bill_dates();
        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found." << std::endl;
            return true;
        }

        std::set<std::string> unique_years;
        for (const auto& month : all_months) {
            if (month.length() >= 4) unique_years.insert(month.substr(0, 4));
        }

        std::cout << "Found " << unique_years.size() << " unique years to process.\n";
        for (const auto& year : unique_years) {
            std::cout << "Exporting summary for " << year << "...";
            if (export_yearly_report(year, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                yearly_stats.success++;
            } else {
                std::cerr << RED_COLOR << " FAILED\n" << RESET_COLOR;
                yearly_stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "\nAn unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }

    std::cout << "\n";
    yearly_stats.print_summary("Yearly Export");
    return yearly_stats.failure == 0;
}


bool QueryFacade::export_all_reports(const std::string& format_name) {
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";
    
    // 先做统一的插件检查
    const bool month_plugin_loaded = m_month_manager.isFormatAvailable(format_name);
    const bool year_plugin_loaded = m_year_manager.isFormatAvailable(format_name);

    if (!month_plugin_loaded || !year_plugin_loaded) {
        std::cerr << "\n" << RED_COLOR << "Export Aborted:" << RESET_COLOR 
                  << " A required format plugin '" << format_name << "' could not be loaded." << std::endl;
        if (!month_plugin_loaded) std::cerr << " -> Missing monthly formatter.\n";
        if (!year_plugin_loaded) std::cerr << " -> Missing yearly formatter.\n";
        return false;
    }

    bool monthly_ok = export_all_monthly_reports(format_name);
    bool yearly_ok = export_all_yearly_reports(format_name);

    bool overall_success = monthly_ok && yearly_ok;
    
    if(overall_success) {
        std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
    } else {
        std::cout << "\n" << RED_COLOR << "Failed: " << RESET_COLOR << "Full report export completed with errors.\n";
    }
    
    return overall_success;
}