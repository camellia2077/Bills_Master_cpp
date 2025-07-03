#include "AppController.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

// Include all necessary functional module interfaces
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
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- Validating: " << file.string() << " ---\n";
            reprocessor.validate_bill(file.string());
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::handle_modification(const std::string& path) {
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
            reprocessor.modify_bill(file.string(), modified_path.string());
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::handle_import(const std::string& path) {
    const std::string db_path = "bills.db";
    std::cout << "Using database file: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- Processing for DB: " << file.string() << " ---\n";
            data_processor.process_and_insert(file.string(), db_path);
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::handle_full_workflow(const std::string& path) {
    std::cout << "--- Auto-Process Workflow Started ---\n";
    try {
        FileHandler file_handler;
        Reprocessor reprocessor("./config");
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_txt_files(path);
        if (files.empty()) return;
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "Processing file: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            
            std::cout << "\n[Step 1/3] Validating bill file...\n";
            if (!reprocessor.validate_bill(file_path.string())) {
                std::cerr << RED_COLOR << "Validation Failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
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
                std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Could not determine year from filename '" << file_path.filename().string() << "'. Saving in root txt_raw directory.\n";
                fs::path target_dir = fs::path("txt_raw");
                fs::create_directory(target_dir);
                modified_path = target_dir / file_path.filename();
            }
            
            std::cout << "\n[Step 2/3] Modifying bill file...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "Modification Failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Modification complete. Modified file saved to '" << modified_path.string() << "'.\n";
            
            std::cout << "\n[Step 3/3] Parsing and inserting into database...\n";
            const std::string db_path = "bills.db";
            if (data_processor.process_and_insert(modified_path.string(), db_path)) {
                std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Database import complete for this file." << "\n";
            } else {
                std::cerr << RED_COLOR << "Database Import Failed" << RESET_COLOR << " for this file." << "\n";
            }
        }
        std::cout << "\n--- Auto-Process Workflow Finished for all processed files ---\n";

    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "An error occurred during the workflow: " << e.what() << std::endl;
    }
}

void AppController::handle_yearly_query(const std::string& year) {
    try {
        QueryFacade facade("bills.db");
        std::string report = facade.get_yearly_summary_report(year);
        std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            try {
                fs::path base_dir("markdown_bills");
                fs::path target_dir = base_dir / "years";
                fs::create_directories(target_dir);
                fs::path output_path = target_dir / (year + ".md");
                
                std::ofstream output_file(output_path);
                if (output_file) {
                    output_file << report;
                    output_file.close();
                    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
                } else {
                    std::cerr << "\n" << RED_COLOR << "Error: " << RESET_COLOR << "Could not open file for writing: " << output_path.string() << std::endl;
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "\n" << RED_COLOR << "Filesystem Error: " << RESET_COLOR << "while saving report: " << e.what() << std::endl;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::handle_monthly_query(const std::string& month) {
    try {
        QueryFacade facade("bills.db");
        std::string report = facade.get_monthly_details_report(month);
        std::cout << report;

        if (report.find("未找到") == std::string::npos) {
            fs::path output_path;
            if (month.length() >= 4) {
                std::string year = month.substr(0, 4);
                // MODIFIED: Path logic updated to save in a fixed "months" folder, categorized by year.
                fs::path target_dir = fs::path("markdown_bills") / "months" / year;
                fs::create_directories(target_dir);
                output_path = target_dir / (month + ".md");
            } else {
                // Fallback for filenames that don't have a clear year
                fs::path target_dir = fs::path("markdown_bills") / "months";
                fs::create_directories(target_dir);
                output_path = target_dir / (month + ".md");
            }

            std::ofstream output_file(output_path);
            if (output_file) {
                output_file << report;
                output_file.close();
                std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
            } else {
                std::cerr << "\n" << RED_COLOR << "Error: " << RESET_COLOR << "Could not open file for writing: " << output_path.string() << std::endl;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
    }
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}