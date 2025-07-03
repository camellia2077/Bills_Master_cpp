#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream> 

// Include all necessary functional module interfaces
#include "Reprocessor.h"
#include "DataProcessor.h" 
#include "QueryDb.h" 
#include "FileHandler.h"
#include "version.h"
#include "common_utils.h"

namespace fs = std::filesystem;

// For UTF-8 output on Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Sets up the console for proper UTF-8 character display.
void setup_console() { 
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

// Prints the help message with all available commands
void print_help(const char* program_name) {
    std::cout << "Bill Reprocessor - A command-line tool for processing bill files.\n\n";
    std::cout << "Usage: " << program_name << " <command> [arguments...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  process, -p <path>          Run the full workflow (validate, modify, import) on a file or directory.\n";
    std::cout << "  validate, -v <path>         Validate a .txt bill file or all .txt files in a directory.\n";
    std::cout << "  modify, -m <path>           Modify a .txt file or all .txt files in a directory.\n";
    std::cout << "                                Output is saved to the 'txt_raw/YYYY/' directory.\n";
    std::cout << "  import, -i <path>           Parse and insert a .txt file or a directory of .txt files into the database.\n";
    std::cout << "  query-year, -qy <year>      Query the annual summary for the given year (e.g., 2025).\n";
    std::cout << "  query-month, -qm <month>    Query the detailed monthly bill for the given month (e.g., 202507).\n\n";
    std::cout << "Options:\n";
    std::cout << "  --version, -V               Display application version information.\n";
    std::cout << "  --help, -h                  Display this help message.\n";
}


int main(int argc, char* argv[]) {
    setup_console();

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    try {
        if (command == "--help" || command == "-h") {
            print_help(argv[0]);
        } 
        else if (command == "--version" || command == "-V") {
            std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
            std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
        }
        else if (command == "process" || command == "-p") {
            if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'process' command.\n";
                return 1;
            }
            FileHandler file_handler;
            Reprocessor reprocessor("./config");
            DataProcessor data_processor;

            std::vector<fs::path> files = file_handler.find_txt_files(argv[2]);
            if (files.empty()) return 0;
            
            std::cout << "--- Starting Full Processing Workflow ---\n";
            for (const auto& file_path : files) {
                std::cout << "\n========================================\n";
                std::cout << "Processing file: " << file_path.string() << "\n";
                std::cout << "========================================\n";

                // Step 1: Validate
                std::cout << "[1/3] Validating...\n";
                if (!reprocessor.validate_bill(file_path.string())) {
                    std::cerr << RED_COLOR << "Validation Failed: " << RESET_COLOR << "Skipping this file.\n";
                    continue;
                }
                std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Validation complete.\n";

                // Step 2: Modify
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

                std::cout << "[2/3] Modifying...\n";
                if (!reprocessor.modify_bill(file_path.string(), modified_path.string())) {
                    std::cerr << RED_COLOR << "Modification Failed: " << RESET_COLOR << "Skipping this file.\n";
                    continue;
                }
                std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Modification complete. Output: " << modified_path.string() << "\n";

                // Step 3: Import (Parse and Insert)
                const std::string db_path = "bills.db";
                std::cout << "[3/3] Importing to database...\n";
                if (!data_processor.process_and_insert(modified_path.string(), db_path)) {
                     std::cerr << RED_COLOR << "Import Failed: " << RESET_COLOR << "Database import failed for " << modified_path.string() << ".\n";
                } else {
                    std::cout << GREEN_COLOR << "Success: " << RESET_COLOR << "Database import complete.\n";
                }
            }
            std::cout << "\n--- Full Workflow Finished ---\n";
        }
        else if (command == "validate" || command == "-v") {
            if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'validate' command.\n";
                return 1;
            }
            FileHandler file_handler;
            Reprocessor reprocessor("./config");
            std::vector<fs::path> files = file_handler.find_txt_files(argv[2]);
            for (const auto& file : files) {
                std::cout << "\n--- Validating: " << file.string() << " ---\n";
                reprocessor.validate_bill(file.string());
            }
        }
        else if (command == "modify" || command == "-m") {
            if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'modify' command.\n";
                return 1;
            }
            FileHandler file_handler;
            Reprocessor reprocessor("./config");
            std::vector<fs::path> files = file_handler.find_txt_files(argv[2]);

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
                    fs::path target_dir = fs::path("txt_raw");
                    fs::create_directory(target_dir);
                    modified_path = target_dir / file.filename();
                }

                std::cout << "\n--- Modifying: " << file.string() << " -> " << modified_path.string() << " ---\n";
                reprocessor.modify_bill(file.string(), modified_path.string());
            }
        }
        else if (command == "import" || command == "-i") {
            if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path argument for 'import' command.\n";
                return 1;
            }
            FileHandler file_handler;
            DataProcessor data_processor;
            const std::string db_path = "bills.db";
            std::cout << "Using database file: " << db_path << "\n";
            std::vector<fs::path> files = file_handler.find_txt_files(argv[2]);
            for (const auto& file : files) {
                std::cout << "\n--- Processing for DB: " << file.string() << " ---\n";
                data_processor.process_and_insert(file.string(), db_path);
            }
        }
        else if (command == "query-year" || command == "-qy") {
            if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> argument for 'query-year' command.\n";
                return 1;
            }
            std::string year = argv[2];
            QueryFacade facade("bills.db");
            std::string report = facade.get_yearly_summary_report(year);
            std::cout << report;

            if (report.find("未找到") == std::string::npos) {
                // MODIFIED: Path logic updated to match main.cpp
                fs::path base_dir("markdown_bills");
                fs::path target_dir = base_dir / "years";
                fs::create_directories(target_dir);
                fs::path output_path = target_dir / (year + ".md");
                
                std::ofstream output_file(output_path);
                if (output_file) {
                    output_file << report;
                    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
                }
            }
        }
        else if (command == "query-month" || command == "-qm") {
             if (argc < 3) {
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> argument for 'query-month' command.\n";
                return 1;
            }
            std::string month = argv[2];
            QueryFacade facade("bills.db");
            std::string report = facade.get_monthly_details_report(month);
            std::cout << report;

            if (report.find("未找到") == std::string::npos) {
                fs::path output_path;
                if (month.length() >= 4) {
                    std::string year = month.substr(0, 4);
                    fs::path target_dir = fs::path("markdown_bills") / year;
                    fs::create_directories(target_dir);
                    output_path = target_dir / (month + ".md");
                } else {
                    fs::path target_dir = fs::path("markdown_bills");
                    fs::create_directory(target_dir);
                    output_path = target_dir / (month + ".md");
                }
                
                std::ofstream output_file(output_path);
                if (output_file) {
                    output_file << report;
                    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << output_path.string() << "\n";
                }
            }
        }
        else {
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown command '" << command << "'\n\n";
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1; 
    }

    return 0; 
}