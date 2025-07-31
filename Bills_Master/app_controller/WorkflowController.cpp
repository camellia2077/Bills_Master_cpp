#include "WorkflowController.h"
#include "reprocessing/Reprocessor.h"
#include "db_insert/DataProcessor.h"
#include "file_handler/FileHandler.h"
#include "ProcessStats.h"
#include "common/common_utils.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>

namespace fs = std::filesystem;

WorkflowController::WorkflowController(const std::string& config_path, const std::string& modified_output_dir)
    : m_config_path(config_path), m_modified_output_dir(modified_output_dir) {}

// Corrected: Belongs to WorkflowController
bool WorkflowController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
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
    return stats.failure == 0;
}

// Corrected: Belongs to WorkflowController
bool WorkflowController::handle_modification(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::string filename_stem = file.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file.filename();
            } else {
                std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Could not determine year from filename '" << file.filename().string() << "'. Saving in root txt_raw directory.\n";
                fs::path target_dir(m_modified_output_dir);
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
    return stats.failure == 0;
}

// Corrected: Belongs to WorkflowController
bool WorkflowController::handle_import(const std::string& path) {
    ProcessStats stats;
    const std::string db_path = "bills.sqlite3";
    std::cout << "Using database file: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_txt_files(path);
        for (const auto& file : files) {
            std::cout << "\n--- Processing for database: " << file.string() << " ---\n";
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
    return stats.failure == 0;
}

// Corrected: Belongs to WorkflowController
bool WorkflowController::handle_full_workflow(const std::string& path) {
    ProcessStats stats;
    std::cout << "--- Automatic processing workflow started ---\n";
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_txt_files(path);
        if (files.empty()) {
            std::cout << "No files found to process.\n";
            stats.print_summary("Full Workflow");
            return true;
        }
        
        for (const auto& file_path : files) {
            std::cout << "\n========================================\n";
            std::cout << "Processing file: " << file_path.string() << "\n";
            std::cout << "========================================\n";
            
            std::cout << "\n[Step 1/3] Validating bill file...\n";
            if (!reprocessor.validate_bill(file_path.string())) {
                std::cerr << RED_COLOR << "Validation failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Validation complete." << "\n";
            
            std::string filename_stem = file_path.stem().string();
            fs::path modified_path;
            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / file_path.filename();
            } else {
                fs::path target_dir(m_modified_output_dir);
                fs::create_directory(target_dir);
                modified_path = target_dir / file_path.filename();
            }

            std::cout << "\n[Step 2/3] Modifying bill file...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "Modification failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Modification complete. Modified file saved to '" << modified_path.string() << "'.\n";
            
            std::cout << "\n[Step 3/3] Parsing and inserting into database...\n";
            const std::string db_path = "bills.sqlite3";
            if (data_processor.process_and_insert(modified_path.string(), db_path)) {
                std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Database import for this file is complete." << "\n";
                stats.success++;
            } else {
                std::cerr << RED_COLOR << "Database import failed" << RESET_COLOR << " for this file." << "\n";
                stats.failure++;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "An error occurred during the workflow: " << e.what() << std::endl;
        stats.failure++;
    }
    stats.print_summary("Full Workflow");
    return stats.failure == 0;
}