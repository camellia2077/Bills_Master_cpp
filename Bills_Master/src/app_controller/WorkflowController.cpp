// app_controller/WorkflowController.cpp

#include "WorkflowController.hpp"
#include "reprocessing/Reprocessor.hpp"
#include "db_insert/DataProcessor.hpp"
#include "file_handler/FileHandler.hpp"
#include "ProcessStats.hpp"
#include "common/common_utils.hpp"

#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>

namespace fs = std::filesystem;

WorkflowController::WorkflowController(const std::string& config_path, const std::string& modified_output_dir)
    : m_config_path(config_path), m_modified_output_dir(modified_output_dir) {}

// ... handle_validation 和 handle_modification 方法保持不变 ...
bool WorkflowController::handle_validation(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        for (const auto& file : files) {
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

bool WorkflowController::handle_modification(const std::string& path) {
    ProcessStats stats;
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        for (const auto& file : files) {
            fs::path modified_path = file;
            modified_path.replace_extension(".json");

            std::string filename_stem = modified_path.stem().string();
            fs::path final_output_path;

            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                final_output_path = target_dir / modified_path.filename();
            } else {
                std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Could not determine year from filename '" << modified_path.filename().string() << "'. Saving in root output directory.\n";
                fs::path target_dir(m_modified_output_dir);
                fs::create_directory(target_dir);
                final_output_path = target_dir / modified_path.filename();
            }
            
            std::cout << "\n--- Modifying: " << file.string() << " -> " << final_output_path.string() << " ---\n";
            if(reprocessor.modify_bill(file.string(), final_output_path.string())) {
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


// [修改] 使用传入的 db_path 参数，而不是硬编码
bool WorkflowController::handle_import(const std::string& path, const std::string& db_path) {
    ProcessStats stats;
    std::cout << "Using database file: " << db_path << "\n";

    try {
        FileHandler file_handler;
        DataProcessor data_processor;
        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".json");
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

// [修改] 使用传入的 db_path 参数
bool WorkflowController::handle_full_workflow(const std::string& path, const std::string& db_path) {
    ProcessStats stats;
    std::cout << "--- Automatic processing workflow started ---\n";
    try {
        FileHandler file_handler;
        Reprocessor reprocessor(m_config_path);
        DataProcessor data_processor;

        std::vector<fs::path> files = file_handler.find_files_by_extension(path, ".txt");
        if (files.empty()) {
            std::cout << "No .txt files found to process.\n";
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
            
            fs::path temp_path = file_path;
            temp_path.replace_extension(".json");
            
            std::string filename_stem = temp_path.stem().string();
            fs::path modified_path;

            if (filename_stem.length() >= 4) {
                std::string year = filename_stem.substr(0, 4);
                fs::path target_dir = fs::path(m_modified_output_dir) / year;
                fs::create_directories(target_dir);
                modified_path = target_dir / temp_path.filename();
            } else {
                fs::path target_dir(m_modified_output_dir);
                fs::create_directory(target_dir);
                modified_path = target_dir / temp_path.filename();
            }

            std::cout << "\n[Step 2/3] Modifying bill file...\n";
            if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                std::cerr << RED_COLOR << "Modification failed" << RESET_COLOR << " for " << file_path.string() << ". Skipping this file." << "\n";
                stats.failure++;
                continue;
            }
            std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Modification complete. Modified file saved to '" << modified_path.string() << "'.\n";
            
            std::cout << "\n[Step 3/3] Parsing and inserting into database...\n";
            // [修改] 确保将正确的 db_path 传递给数据库处理器
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