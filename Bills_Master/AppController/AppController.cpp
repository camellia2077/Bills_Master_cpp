#include "AppController.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <set>
#include <stdexcept> // for std::invalid_argument, std::out_of_range

#include "Reprocessor.h"
#include "DataProcessor.h"
#include "QueryDb.h"
#include "FileHandler.h"
#include "version.h"
#include "common_utils.h"

namespace fs = std::filesystem;

AppController::AppController() {
    // Constructor can be used for one-time setup if needed in the future.
}

void AppController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- Validating: " << file.string() << " ---\n";
            if (reprocessor.validate_bill(file.string())) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Validation");
}

void AppController::handle_modification(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::string filename_stem = file.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path("txt_raw") / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file.filename();
            } else {
                std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Could not determine year from filename '" << file.filename().string() << "'. Saving in root txt_raw directory.\n";
                fs::path target_dir("txt_raw");
                fs::create_directory(target_dir);
                modified_path = target_dir / file.filename();
            }
            
            std::cout << "\n--- Modifying: " << file.string() << " -> " << modified_path.string() << " ---\n";
            if(reprocessor.modify_bill(file.string(), modified_path.string())) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Modification");
}

void AppController::handle_import(const std::string& path) {
    ProcessStats stats;
    const std::string db_path = "bills.db";
    std::cout << "Using database file: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- Processing for DB: " << file.string() << " ---\n";
            if (data_processor.process_and_insert(file.string(), db_path)) {
                stats.success++;
            } else {
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Database Import");
}

void AppController::handle_full_workflow(const std::string& path) {
    ProcessStats stats;
    std::cout << "--- Auto-Process Workflow Started ---\n";
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_txt_files(path);
        if (files.empty()) {
            std::cout << "No files found to process.\n";
            return;
        }
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "Processing file: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            
            std::cout << "\n[Step 1/3] Validating bill file...\n";
            if (!reprocessor.validate_bill(file_path.string())) {
                std::cerr << RED_COLOR << "Validation Failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Validation complete." << "\n";
            
            std::string filename_stem = file_path.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path("txt_raw") / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file_path.filename();
            } else {
                fs::path target_dir = fs::path("txt_raw");
                fs::create_directory(target_dir);
                modified_path = target_dir / file_path.filename();
            }

            std::cout << "\n[Step 2/3] Modifying bill file...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "Modification Failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Modification complete. Modified file saved to '" << modified_path.string() << "'.\n";
            
            std::cout << "\n[Step 3/3] Parsing and inserting into database...\n";
            const std::string db_path = "bills.db";
            if (data_processor.process_and_insert(modified_path.string(), db_path)) {
                std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Database import complete for this file." << "\n";
                stats.success++;
            } else {
                std::cerr << RED_COLOR << "Database Import Failed" << RESET_COLOR << " for this file." << "\n";
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "An error occurred during the workflow: " << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Full Workflow");
}

bool AppController::handle_yearly_query(const std::string& year_str, bool is_part_of_export_all) {
    try {
        int year = std::stoi(year_str);

        QueryFacade facade("bills.db");
        std::string report = facade.get_yearly_summary_report(year);
        
        if (!is_part_of_export_all) {
            std::cout << report;
        }

        if (report.find("未找到") != std::string::npos) {
            return true; // Considered a success (operation completed), just no data.
        }

        fs::path target_dir = fs::path("markdown_bills") / "years";
        fs::create_directories(target_dir);
        fs::path output_path = target_dir / (year_str + ".md");
        
        std::ofstream output_file(output_path);
        if (output_file) {
            output_file << report;
            if (!is_part_of_export_all) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
            return true;
        } else {
            std::cerr << "\n" << RED_COLOR << "Error: " << RESET_COLOR << "Could not open file for writing: " << output_path.string() << std::endl;
            return false;
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << "Invalid year format. Please provide a 4-digit year (e.g., 2025).\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}

bool AppController::handle_monthly_query(const std::string& month_str, bool is_part_of_export_all) {
    try {
        if (month_str.length() != 6) {
            throw std::invalid_argument("Invalid month format.");
        }
        int year = std::stoi(month_str.substr(0, 4));
        int month = std::stoi(month_str.substr(4, 2));

        QueryFacade facade("bills.db");
        std::string report = facade.get_monthly_details_report(year, month);

        if (!is_part_of_export_all) {
            std::cout << report;
        }
        
        if (report.find("未找到") != std::string::npos) {
            return true; // Considered a success (operation completed), just no data.
        }

        fs::path output_path;
        if (month_str.length() >= 4) {
            std::string year_dir = month_str.substr(0, 4);
            fs::path target_dir = fs::path("markdown_bills") / "months" / year_dir;
            fs::create_directories(target_dir);
            output_path = target_dir / (month_str + ".md");
        } else {
            fs::path target_dir = fs::path("markdown_bills") / "months";
            fs::create_directories(target_dir);
            output_path = target_dir / (month_str + ".md");
        }

        std::ofstream output_file(output_path);
        if (output_file) {
            output_file << report;
            if (!is_part_of_export_all) {
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            }
            return true;
        } else {
            std::cerr << "\n" << RED_COLOR << "Error: " << RESET_COLOR << "Could not open file for writing: " << output_path.string() << std::endl;
            return false;
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << "Invalid month format. Please provide a 6-digit month (e.g., 202506).\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}

void AppController::handle_export_all() {
    ProcessStats monthly_stats;
    ProcessStats yearly_stats;
    
    std::cout << "\n--- Starting Full Report Export ---\n";
    try {
        QueryFacade facade("bills.db");
        std::vector<std::string> all_months = facade.get_all_bill_dates();

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
            if (handle_monthly_query(month, true)) {
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
            if (handle_yearly_query(year, true)) {
                std::cout << GREEN_COLOR << " OK\n" << RESET_COLOR;
                yearly_stats.success++;
            } else {
                std::cout << RED_COLOR << " FAILED\n" << RESET_COLOR;
                yearly_stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Export Failed: " << RESET_COLOR << e.what() << std::endl;
        yearly_stats.failure = 1; // Mark the whole operation as a failure
    }
    
    monthly_stats.print_summary("Monthly Export");
    yearly_stats.print_summary("Yearly Export");
    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Full report export completed.\n";
}