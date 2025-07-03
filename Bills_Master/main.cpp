#include <iostream>
#include <string>
#include <limits>
#include <filesystem>
#include <fstream> 
#include <vector>

// 包含我们所有的功能模块接口
#include "Reprocessor.h"
#include "DataProcessor.h" 
#include "QueryDb.h" 
#include "FileHandler.h"

#include "version.h"
#include "common_utils.h" // 颜色 

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

void print_menu() {
    std::cout << "\n===== Bill Reprocessor Menu =====\n"; 
    std::cout << "1. Validate Bill File(s)\n"; 
    std::cout << "2. Modify Bill File(s)\n"; 
    std::cout << "3. Parse and Insert Bill(s) to Database\n"; 
    std::cout << "4. Query Yearly Summary and Export\n";
    std::cout << "5. Query Monthly Details and Export\n";
    std::cout << "6. Auto-Process Full Workflow (File or Directory)\n";
    std::cout << "7. version\n";
    std::cout << "8. Exit\n";
    std::cout << "=================================\n"; 
    std::cout << "Enter your choice: "; 
}

int main() {
    setup_console(); 

    std::cout << "Welcome to the Bill Reprocessor! (UTF-8 enabled)\n"; 

    try {
        Reprocessor reprocessor("./config"); 
        DataProcessor data_processor; 
        FileHandler file_handler;

        int choice = 0;
        while (choice != 8) { 
            print_menu();
            std::cin >> choice;

            if (std::cin.fail()) { 
                std::cin.clear(); 
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
                std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid input. Please enter a number." << "\n"; 
                choice = 0; 
                continue; 
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

            std::string user_path; 

            switch (choice) {
                case 1: { 
                    std::cout << "Enter path to a .txt file or a directory for validation: ";
                    std::getline(std::cin, user_path);
                    if (user_path.empty()) {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Path cannot be empty." << "\n";
                        break;
                    }

                    try {
                        std::vector<fs::path> files = file_handler.find_txt_files(user_path);
                        for (const auto& file : files) {
                            std::cout << "\n--- Validating: " << file.string() << " ---\n";
                            reprocessor.validate_bill(file.string());
                        }
                    } catch (const std::runtime_error& e) {
                        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
                    }
                    break; 
                }
                case 2: { 
                    std::cout << "Enter path to a .txt file or a directory for modification: ";
                    std::getline(std::cin, user_path);
                    if (user_path.empty()) {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Path cannot be empty." << "\n";
                        break;
                    }

                    try {
                        std::vector<fs::path> files = file_handler.find_txt_files(user_path);
                        const std::string output_dir_str = "txt_raw";
                        fs::create_directory(output_dir_str);

                        for (const auto& file : files) {
                            fs::path modified_path = fs::path(output_dir_str) / file.filename();
                            std::cout << "\n--- Modifying: " << file.string() << " -> " << modified_path.string() << " ---\n";
                            reprocessor.modify_bill(file.string(), modified_path.string());
                        }
                    } catch (const std::runtime_error& e) {
                        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
                    }
                    break; 
                }
                case 3: { 
                    std::cout << "Enter path to a .txt file or a directory to parse and insert: ";
                    std::getline(std::cin, user_path);
                    if (user_path.empty()) {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Path cannot be empty." << "\n";
                        break;
                    }

                    const std::string db_path = "bills.db";
                    std::cout << "Using database file: " << db_path << "\n";

                    try {
                        std::vector<fs::path> files = file_handler.find_txt_files(user_path);
                        for (const auto& file : files) {
                            std::cout << "\n--- Processing for DB: " << file.string() << " ---\n";
                            data_processor.process_and_insert(file.string(), db_path);
                        }
                    } catch (const std::runtime_error& e) {
                        std::cerr << RED_COLOR << "Error: " << RESET_COLOR << e.what() << std::endl;
                    }
                    break; 
                }
                
                case 4: {
                    std::string year;
                    std::cout << "Enter year to query (e.g., 2025): ";
                    std::getline(std::cin, year);
                    if (!year.empty()) {
                        try {
                            QueryFacade facade("bills.db");
                            std::string report = facade.get_yearly_summary_report(year);
                            std::cout << report;

                            if (report.find("未找到") == std::string::npos) {
                                try {
                                    fs::path base_dir("markdown_bills");
                                    fs::path target_dir = base_dir / "year";
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
                    } else {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Year cannot be empty." << "\n";
                    }
                    break;
                }
                case 5: {
                    std::string month;
                    std::cout << "Enter month to query (e.g., 202506): ";
                    std::getline(std::cin, month);
                     if (!month.empty()) {
                        try {
                            QueryFacade facade("bills.db");
                            std::string report = facade.get_monthly_details_report(month);
                            std::cout << report;

                            if (report.find("未找到") == std::string::npos) {
                                const std::string output_dir = "markdown_bills";
                                fs::create_directory(output_dir);
                                std::string filename = output_dir + "/" + month + ".md";
                                
                                std::ofstream output_file(filename);
                                if (output_file) {
                                    output_file << report;
                                    output_file.close();
                                    std::cout << "\n" << GREEN_COLOR << "Success: " << RESET_COLOR << "Report also saved to " << filename << "\n";
                                } else {
                                    std::cerr << "\n" << RED_COLOR << "Error: " << RESET_COLOR << "Could not open file for writing: " << filename << std::endl;
                                }
                            }
                        } catch (const std::runtime_error& e) {
                            std::cerr << RED_COLOR << "Query Failed: " << RESET_COLOR << e.what() << std::endl;
                        }
                    } else {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Month cannot be empty." << "\n";
                    }
                    break;
                }
                case 6: {
                    std::cout << "--- Auto-Process Workflow Started ---\n";
                    std::cout << "Enter path to a source .txt file or a directory containing .txt files: ";
                    std::getline(std::cin, user_path);
                    if (user_path.empty()) {
                        std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Path cannot be empty. Aborting." << "\n";
                        break;
                    }

                    try {
                        std::vector<fs::path> files = file_handler.find_txt_files(user_path);
                        if (files.empty()) break;
                        
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

                            const std::string output_dir = "txt_raw";
                            fs::create_directory(output_dir);
                            fs::path modified_path = fs::path(output_dir) / file_path.filename();
                            
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
                    break;
                }
                case 7:
                    std::cout << "BillsMaster Version: " << AppInfo::VERSION << std::endl;
                    std::cout << "Last Updated: " << AppInfo::LAST_UPDATED << std::endl;
                    break;
                case 8:
                    std::cout << "Exiting program. Goodbye!\n";
                    return 0; // Exit the loop and program
                default:
                    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid choice. Please select a number from the menu." << "\n"; 
                    break;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1; 
    }

    return 0; 
}