#include "QueryDb.h"
#include "year/YearlyReportGenerator.h"
#include "month/MonthlyReportGenerator.h"
#include "ProcessStats.h"
#include "common_utils.h"
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

// --- 构造函数 / 析构函数 (无变化) ---
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
std::string QueryFacade::get_yearly_summary_report(int year, ReportFormat format) {
    YearlyReportGenerator generator(m_db);
    return generator.generate(year, format);
}

std::string QueryFacade::get_monthly_details_report(int year, int month, ReportFormat format) {
    MonthlyReportGenerator generator(m_db);
    return generator.generate(year, month, format);
}

// --- 数据获取方法 (无变化) ---
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

void QueryFacade::export_yearly_report(const std::string& year_str, ReportFormat format, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = get_yearly_summary_report(year, format);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            // --- 核心改动：增加对 Typst 的处理 ---
            std::string extension;
            std::string base_dir;
            switch(format) {
                case ReportFormat::LATEX:
                    extension = ".tex";
                    base_dir = "latex_bills";
                    break;
                case ReportFormat::TYPST:
                    extension = ".typ";
                    base_dir = "typst_bills";
                    break;
                case ReportFormat::MARKDOWN:
                default:
                    extension = ".md";
                    base_dir = "markdown_bills";
                    break;
            }
            
            fs::path target_dir = fs::path(base_dir) / "years";
            fs::path output_path = target_dir / (year_str + extension);
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        }
    }
}

void QueryFacade::export_monthly_report(const std::string& month_str, ReportFormat format, bool suppress_output) {
    try {
        if (month_str.length() != 6) {
            throw std::invalid_argument("Invalid month format.");
        }
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));

        std::string report = get_monthly_details_report(year, month, format);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            // --- 核心改动：增加对 Typst 的处理 ---
            std::string extension;
            std::string base_dir;
            switch(format) {
                case ReportFormat::LATEX:
                    extension = ".tex";
                    base_dir = "latex_bills";
                    break;
                case ReportFormat::TYPST:
                    extension = ".typ";
                    base_dir = "typst_bills";
                    break;
                case ReportFormat::MARKDOWN:
                default:
                    extension = ".md";
                    base_dir = "markdown_bills";
                    break;
            }

            fs::path target_dir = fs::path(base_dir) / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        }
    }
}

void QueryFacade::export_all_reports(ReportFormat format) {
    ProcessStats monthly_stats, yearly_stats;
    
    // --- 核心改动：增加对 Typst 的处理 ---
    std::string format_name;
    switch(format) {
        case ReportFormat::LATEX:
            format_name = "LaTeX";
            break;
        case ReportFormat::TYPST:
            format_name = "Typst";
            break;
        case ReportFormat::MARKDOWN:
        default:
            format_name = "Markdown";
            break;
    }
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
            export_monthly_report(month, format, true);
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
            export_yearly_report(year, format, true);
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