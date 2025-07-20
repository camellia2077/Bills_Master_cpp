#include "ReportExporter.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <vector>
#include "common_utils.h"

namespace fs = std::filesystem;

ReportExporter::ReportExporter(const std::string& db_path)
    : m_query_facade(db_path), m_db_path(db_path) {}

void ReportExporter::save_report(const std::string& report_content, const std::string& file_path_str) {
    fs::path file_path(file_path_str);
    fs::create_directories(file_path.parent_path());

    std::ofstream output_file(file_path);
    if (!output_file) {
        throw std::runtime_error("Could not open file for writing: " + file_path.string());
    }
    output_file << report_content;
}

bool ReportExporter::export_yearly_report(const std::string& year_str, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = m_query_facade.get_yearly_summary_report(year);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            fs::path target_dir = fs::path("markdown_bills") / "years";
            fs::path output_path = target_dir / (year_str + ".md");
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

bool ReportExporter::export_monthly_report(const std::string& month_str, bool suppress_output) {
    try {
        if (month_str.length() != 6) {
            throw std::invalid_argument("Invalid month format.");
        }
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));

        std::string report = m_query_facade.get_monthly_details_report(year, month);

        if (!suppress_output) {
            std::cout << report;
        }

        if (report.find("未找到") == std::string::npos) {
            fs::path target_dir = fs::path("markdown_bills") / "months" / month_str.substr(0, 4);
            fs::path output_path = target_dir / (month_str + ".md");
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

void ReportExporter::export_all_reports() {
    ProcessStats monthly_stats, yearly_stats;
    std::cout << "\n--- Starting Full Report Export ---\n";

    try {
        std::vector<std::string> all_months = m_query_facade.get_all_bill_dates();

        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found in the database. Nothing to export.\n";
            return;
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
            if (export_monthly_report(month, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                monthly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                monthly_stats.failure++;
            }
        }

        std::cout << "\n--- Exporting Yearly Reports ---\n";
        for (const auto& year : unique_years) {
            std::cout << "Exporting summary for " << year << "...";
            if (export_yearly_report(year, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                yearly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                yearly_stats.failure++;
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1;
    }

    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
}