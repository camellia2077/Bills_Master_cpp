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
        throw std::runtime_error("无法打开数据库: " + errmsg);
    }
}

QueryFacade::~QueryFacade() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

// --- 报告生成方法 ---

std::string QueryFacade::get_yearly_summary_report(int year) {
    YearlyReportGenerator generator(m_db);
    return generator.generate(year);
}

// 现在将格式参数传递给底层的生成器
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
        throw std::runtime_error("准备查询所有日期 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db)));
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
        throw std::runtime_error("无法打开文件进行写入: " + file_path.string());
    }
    output_file << report_content;
}

// 年报导出暂时只支持 Markdown
void QueryFacade::export_yearly_report(const std::string& year_str, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = get_yearly_summary_report(year);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            fs::path target_dir = fs::path("markdown_bills") / "years";
            fs::path output_path = target_dir / (year_str + ".md");
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "成功: " << RESET_COLOR << "报告已保存至 " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "查询失败: " << RESET_COLOR << e.what() << std::endl;
        }
    }
}

// 月报导出已更新，支持不同格式
void QueryFacade::export_monthly_report(const std::string& month_str, ReportFormat format, bool suppress_output) {
    try {
        if (month_str.length() != 6) {
            throw std::invalid_argument("月份格式无效。");
        }
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));

        // 调用更新后的 get_monthly_details_report
        std::string report = get_monthly_details_report(year, month, format);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            // 根据格式确定文件扩展名和目录
            std::string extension = (format == ReportFormat::LATEX) ? ".tex" : ".md";
            std::string base_dir = (format == ReportFormat::LATEX) ? "latex_bills" : "markdown_bills";

            fs::path target_dir = fs::path(base_dir) / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + extension);
            
            save_report(report, output_path.string());
            if (!suppress_output) {
                std::cout << "\n" << GREEN_COLOR << "成功: " << RESET_COLOR << "报告已保存至 " << output_path.string() << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (!suppress_output) {
            std::cerr << RED_COLOR << "查询失败: " << RESET_COLOR << e.what() << std::endl;
        }
    }
}

// '导出所有' 功能现在也可以指定格式
void QueryFacade::export_all_reports(ReportFormat format) {
    ProcessStats monthly_stats, yearly_stats;
    std::string format_name = (format == ReportFormat::LATEX) ? "LaTeX" : "Markdown";
    std::cout << "\n--- 开始导出所有报告 (" << format_name << " 格式) ---\n";

    try {
        std::vector<std::string> all_months = get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "警告: " << RESET_COLOR << "数据库中未找到数据，无法导出。\n";
            return;
        }

        std::cout << "发现 " << all_months.size() << " 个独立月份需要处理。\n";
        
        // --- 导出月度报告 ---
        std::cout << "\n--- 导出月度报告 ---\n";
        for (const auto& month : all_months) {
            std::cout << "正在导出报告 " << month << "...";
            export_monthly_report(month, format, true); // 传递格式参数
            std::cout << GREEN_COLOR << " 完成\n" << RESET_COLOR;
            monthly_stats.success++;
        }

        // --- 导出年度报告 (注意：年度报告仍为 Markdown) ---
        if (format == ReportFormat::MARKDOWN) {
            std::cout << "\n--- 导出年度报告 (Markdown) ---\n";
            std::set<std::string> unique_years;
            for (const auto& month : all_months) {
                if (month.length() >= 4) {
                    unique_years.insert(month.substr(0, 4));
                }
            }
            for (const auto& year : unique_years) {
                std::cout << "正在导出摘要 " << year << "...";
                export_yearly_report(year, true);
                std::cout << GREEN_COLOR << " 完成\n" << RESET_COLOR;
                yearly_stats.success++;
            }
        } else {
             std::cout << "\n--- 年度报告导出已跳过 (仅支持 Markdown) ---\n";
        }


    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "导出失败: " << RESET_COLOR << e.what() << std::endl;
        monthly_stats.failure = 1; // 标记操作失败
    }

    monthly_stats.print_summary("月度报告导出");
    if (format == ReportFormat::MARKDOWN) {
        yearly_stats.print_summary("年度报告导出");
    }
    std::cout << "\n" << GREEN_COLOR << "成功: " << RESET_COLOR << "所有报告导出完成。\n";
}