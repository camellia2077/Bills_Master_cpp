
#include "ReportExporter.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <vector>
#include "common/common_utils.h"

namespace fs = std::filesystem;

// Update constructor to pass both arguments to the m_query_facade member
ReportExporter::ReportExporter(const std::string& db_path, const std::string& plugin_path)
    : m_query_facade(db_path, plugin_path), m_db_path(db_path) {}

void ReportExporter::save_report(const std::string& report_content, const std::string& file_path_str) {
    fs::path file_path(file_path_str);
    fs::create_directories(file_path.parent_path());

    std::ofstream output_file(file_path);
    if (!output_file) {
        throw std::runtime_error("Could not open file for writing: " + file_path.string());
    }
    output_file << report_content;
}

// **修改1：更新函数签名，接收 format_name**
bool ReportExporter::export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        // **修改2：将 format_name 传递给 facade**
        std::string report = m_query_facade.get_yearly_summary_report(year, format_name);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            // **修改3：动态构建路径和扩展名**
            std::string extension = "." + format_name;
            std::string base_dir = format_name + "_bills";
            fs::path target_dir = fs::path(base_dir) / "years";
            fs::path output_path = target_dir / (year_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        }
        return false;
    }
}

// **修改1：更新函数签名，接收 format_name**
bool ReportExporter::export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output) {
    try {
        if (month_str.length() != 6) {
            throw std::invalid_argument("Invalid month format.");
        }
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));

        // **修改2：将 format_name 传递给 facade**
        std::string report = m_query_facade.get_monthly_details_report(year, month, format_name);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            // **修改3：动态构建路径和扩展名**
            std::string extension = "." + format_name;
            std::string base_dir = format_name + "_bills";
            fs::path target_dir = fs::path(base_dir) / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);

            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        }
        return false;
    }
}

// **修改：返回类型从 void 变为 bool**
bool ReportExporter::export_all_reports(const std::string& format_name) {
    ProcessStats monthly_stats, yearly_stats;
    bool overall_success = true; // 新增：追踪整体操作是否成功
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";

    try {
        std::vector<std::string> all_months = m_query_facade.get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found in the database. Nothing to export.\n";
            return true; // 没有数据可导，但操作本身是成功的
        }

        std::cout << "Found " << all_months.size() << " unique months to process.\n";
        std::set<std::string> unique_years;
        for (const auto& month : all_months) {
            if (month.length() >= 4) {
                unique_years.insert(month.substr(0, 4));
            }
        }

        std::cout << "\n--- Exporting Monthly Reports ---\n";
        for (const auto& month : all_months) {
            std::cout << "Exporting report for " << month << "...";
            if (export_monthly_report(month, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                monthly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                monthly_stats.failure++;
                overall_success = false; // **修改：如果失败，则更新标志**
            }
        }

        std::cout << "\n--- Exporting Yearly Reports ---\n";
        for (const auto& year : unique_years) {
            std::cout << "Exporting summary for " << year << "...";
            if (export_yearly_report(year, format_name, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                yearly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                yearly_stats.failure++;
                overall_success = false; // **修改：如果失败，则更新标志**
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1;
        overall_success = false; // **修改：捕获到异常也意味着失败**
    }

    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    
    // **修改：根据最终状态打印不同的消息**
    if(overall_success) {
        std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
    } else {
        std::cout << "\n" << RED_COLOR << "Failed: " << RESET_COLOR << "Full report export completed with errors.\n";
    }

    return overall_success; // **修改：返回最终的成功状态**
}