// query/core/QueryFacade.cpp
#include "QueryFacade.hpp"
#include "common/ProcessStats.hpp"
#include "common/common_utils.hpp"
#include <stdexcept>
#include <iostream>
#include <set>

// Constructor 1: Load plugins from a directory
QueryFacade::QueryFacade(const std::string& db_path, const std::string& plugin_directory_path, const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_db(nullptr)
{
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
    }
    m_monthly_generator = std::make_unique<MonthlyReportGenerator>(m_db, plugin_directory_path);
    m_yearly_generator = std::make_unique<YearlyReportGenerator>(m_db, plugin_directory_path);
    m_report_exporter = std::make_unique<ReportExporter>(export_base_dir, format_folder_names);
    m_metadata_reader = std::make_unique<BillMetadataReader>(m_db);
}

// Constructor 2: Load plugins from a list of files
QueryFacade::QueryFacade(const std::string& db_path, const std::vector<std::string>& plugin_paths, const std::string& export_base_dir, const std::map<std::string, std::string>& format_folder_names)
    : m_db(nullptr)
{
    if (sqlite3_open_v2(db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
    }
    m_monthly_generator = std::make_unique<MonthlyReportGenerator>(m_db, plugin_paths);
    m_yearly_generator = std::make_unique<YearlyReportGenerator>(m_db, plugin_paths);
    m_report_exporter = std::make_unique<ReportExporter>(export_base_dir, format_folder_names);
    m_metadata_reader = std::make_unique<BillMetadataReader>(m_db);
}

QueryFacade::~QueryFacade() {
    if (m_db) sqlite3_close(m_db);
}

bool QueryFacade::export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output) {
    try {
        int year = std::stoi(year_str);
        std::string report = m_yearly_generator->generate(year, format_name);

        if (!suppress_output) std::cout << report;
        
        m_report_exporter->export_yearly(report, year_str, format_name);
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}

bool QueryFacade::export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output) {
    try {
        if (month_str.length() != 6) throw std::invalid_argument("Invalid month format.");
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));
        std::string report = m_monthly_generator->generate(year, month, format_name);

        if (!suppress_output) std::cout << report;

        m_report_exporter->export_monthly(report, month_str, format_name);
        return true;
    } catch (const std::exception& e) {
        if (!suppress_output) std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}

bool QueryFacade::export_by_date(const std::string& date_str, const std::string& format_name) {
    if (date_str.length() == 4) {
        return export_yearly_report(date_str, format_name, false);
    } else if (date_str.length() == 6) {
        return export_monthly_report(date_str, format_name, false);
    } else {
        std::cerr << RED_COLOR << "Error:" << RESET_COLOR << " Invalid date format for export: '" << date_str << "'. Please use YYYY or YYYYMM.\n";
        return false;
    }
}

bool QueryFacade::export_by_date_range(const std::string& start_date, const std::string& end_date, const std::string& format_name) {
    if (start_date.length() != 6 || end_date.length() != 6 || start_date > end_date) {
        std::cerr << RED_COLOR << "Error:" << RESET_COLOR << " Invalid date range. Use YYYYMM format and ensure start_date is not after end_date.\n";
        return false;
    }
    
    std::cout << "\n--- Exporting monthly reports from " << start_date << " to " << end_date << " (" << format_name << " format) ---\n";
    
    ProcessStats stats;
    try {
        std::vector<std::string> all_months = m_metadata_reader->get_all_bill_dates();
        std::vector<std::string> months_to_export;
        for (const auto& month : all_months) {
            if (month >= start_date && month <= end_date) {
                months_to_export.push_back(month);
            }
        }

        if (months_to_export.empty()) {
            std::cout << YELLOW_COLOR << "Warning:" << RESET_COLOR << " No data found within the specified date range.\n";
            return true;
        }

        for (const auto& month : months_to_export) {
            std::cout << "Exporting report for " << month << "...";
            if (export_monthly_report(month, format_name, true)) {
                stats.success++;
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            } else {
                stats.failure++;
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "An unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }

    stats.print_summary("Date Range Export");
    return stats.failure == 0;
}

bool QueryFacade::export_all_monthly_reports(const std::string& format_name) {
    ProcessStats stats;
    std::cout << "\n--- Starting Monthly Report Export (" << format_name << " format) ---\n";
    try {
        std::vector<std::string> all_months = m_metadata_reader->get_all_bill_dates();
        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found.\n";
            return true;
        }

        for (const auto& month : all_months) {
            std::cout << "Exporting report for " << month << "...";
            if (export_monthly_report(month, format_name, true)) {
                stats.success++;
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            } else {
                stats.failure++;
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "\nAn unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
    stats.print_summary("Monthly Export");
    return stats.failure == 0;
}

bool QueryFacade::export_all_yearly_reports(const std::string& format_name) {
    ProcessStats stats;
    std::cout << "\n--- Starting Yearly Report Export (" << format_name << " format) ---\n";
    try {
        std::vector<std::string> all_months = m_metadata_reader->get_all_bill_dates();
        if (all_months.empty()) {
            std::cout << YELLOW_COLOR << "Warning: " << RESET_COLOR << "No data found.\n";
            return true;
        }
        
        std::set<std::string> unique_years;
        for (const auto& month : all_months) {
            if (month.length() >= 4) unique_years.insert(month.substr(0, 4));
        }

        for (const auto& year : unique_years) {
            std::cout << "Exporting summary for " << year << "...";
            if (export_yearly_report(year, format_name, true)) {
                stats.success++;
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
            } else {
                stats.failure++;
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "\nAn unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
    stats.print_summary("Yearly Export");
    return stats.failure == 0;
}

bool QueryFacade::export_all_reports(const std::string& format_name) {
    std::cout << "\n--- Starting Full Report Export (" << format_name << " format) ---\n";
    bool monthly_ok = export_all_monthly_reports(format_name);
    bool yearly_ok = export_all_yearly_reports(format_name);
    bool overall_success = monthly_ok && yearly_ok;

    if (overall_success) {
        std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
    } else {
        std::cout << "\n" << RED_COLOR << "Failed: " << RESET_COLOR << "Full report export completed with errors.\n";
    }
    return overall_success;
}