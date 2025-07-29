#include "AppController.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <string>
#include <map> // [修改] 确保包含 map 头文件

#include "reprocessing/Reprocessor.h"
#include "db_insert/DataProcessor.h"
#include "query/QueryDb.h"
#include "file_handler/FileHandler.h"
#include "common/version.h"
#include "common/common_utils.h"

namespace fs = std::filesystem;


AppController::AppController(const std::string& db_path,const std::string& config_path)
    : m_db_path(db_path),
    m_config_path(config_path){
    // 定义动态库列表
    m_plugin_files = {
        "plugins/md_month_formatter.dll",
        "plugins/md_year_formatter.dll",

        "plugins/tex_month_formatter.dll",
        "plugins/tex_year_formatter.dll",

        "plugins/rst_month_formatter.dll",
        "plugins/rst_year_formatter.dll",

        "plugins/typ_month_formatter.dll", 
        "plugins/typ_year_formatter.dll"
    };

    // 配置导出目录
    m_export_base_dir = "exported_files";
    m_format_folder_names = {
        {"md", "Markdown_bills"}, // md格式的报告放在这个文件夹
        {"tex", "LaTeX_bills"},     // tex格式的报告放在这个文件夹
        {"rst", "reST_bills"},      // rst格式的报告放在这个文件夹
        {"typ", "typst_bills"}      // typ格式的报告放在这个文件夹
    };
}

bool AppController::handle_validation(const std::string& path) {
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

bool AppController::handle_modification(const std::string& path) {
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
    return stats.failure == 0;
}

bool AppController::handle_import(const std::string& path) {
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

bool AppController::handle_full_workflow(const std::string& path) {
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

bool AppController::handle_export(const std::string& type, const std::vector<std::string>& values, const std::string& format_str) {
    bool success = false;
    try {
        QueryFacade facade(m_db_path, m_plugin_files, m_export_base_dir, m_format_folder_names);

        if (type == "all") {
            success = facade.export_all_reports(format_str);
        } else if (type == "all_months") {
            success = facade.export_all_monthly_reports(format_str);
        } else if (type == "all_years") {
            success = facade.export_all_yearly_reports(format_str);
        } else if (type == "date") {
            if (values.empty()) {
                throw std::runtime_error("At least one date string must be provided for 'date' export.");
            }
            if (values.size() == 1) { // 单个日期
                success = facade.export_by_date(values[0], format_str);
            } else if (values.size() == 2) { // 日期区间
                success = facade.export_by_date_range(values[0], values[1], format_str);
            } else {
                throw std::runtime_error("For 'date' export, please provide one (YYYY or YYYYMM) or two (YYYYMM YYYYMM) date values.");
            }
        } else if (type == "year") {
            if (values.empty() || values[0].empty()) {
                throw std::runtime_error("A year must be provided to export a yearly report.");
            }
            success = facade.export_yearly_report(values[0], format_str);
        } else if (type == "month") {
            if (values.empty() || values[0].empty()) {
                throw std::runtime_error("A month (YYYYMM) must be provided to export a monthly report.");
            }
            success = facade.export_monthly_report(values[0], format_str);
        } else {
            throw std::runtime_error("Unknown export type: " + type);
        }
    } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "Export failed: " << RESET_COLOR << e.what() << std::endl;
        return false;
    }
    return success;
}

void AppController::display_version() {
    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
}