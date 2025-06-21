#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

#include "LineValidator.h"
#include "Bill_Parser.h"
#include "DatabaseInserter.h"
#include "BillReporter.h"

namespace fs = std::filesystem;
struct Colors {
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string reset = "\033[0m";
};

void show_menu() {
    std::cout << "\n===== Bill Management System =====\n";
    std::cout << "0. Import data from .txt file(s)\n";
    std::cout << "1. Annual consumption summary\n";
    std::cout << "2. Detailed monthly bill\n";
    std::cout << "3. Export monthly bill (machine-readable)\n";
    std::cout << "4. Annual category statistics\n";
    std::cout << "5. Exit\n";
    std::cout << "==================================\n";
    std::cout << "Enter your choice: ";
}

void clear_cin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void handle_import_process(const std::string& db_file) {
    std::string user_path_str;
    std::cout << "Enter the path to a .txt file or a directory: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, user_path_str);

    fs::path user_path(user_path_str);
    std::vector<fs::path> files_to_process;
    Colors colors;

    try {
        if (!fs::exists(user_path)) {
            std::cerr << "Error: Path does not exist: " << user_path.string() << std::endl;
            return;
        }

        if (fs::is_regular_file(user_path)) {
            if (user_path.extension() == ".txt") files_to_process.push_back(user_path);
            else std::cerr << "Error: The provided file is not a .txt file." << std::endl;
        } else if (fs::is_directory(user_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(user_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                    files_to_process.push_back(entry.path());
                }
            }
        } else {
            std::cerr << "Error: The provided path is not a regular file or directory." << std::endl;
            return;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return;
    }

    if (files_to_process.empty()) {
        std::cout << "No .txt files found to process." << std::endl;
        return;
    }

    int successful_files = 0;
    int failed_files = 0;
    auto total_parsing_duration = std::chrono::nanoseconds(0);
    auto total_insertion_duration = std::chrono::nanoseconds(0);

    DatabaseInserter inserter(db_file);
    
    // ==========================================================
    // == 依赖注入发生在这里 ==
    // 1. 创建 LineValidator 实例
    LineValidator validator;
    // 2. 将 validator 注入到 Bill_Parser 的构造函数中
    Bill_Parser parser(validator);
    // ==========================================================

    bool transaction_active = false;

    try {
        inserter.create_database();
        
        inserter.begin_transaction();
        transaction_active = true;

        for (const auto& file_path : files_to_process) {
            parser.reset();

            std::vector<ParsedRecord> current_file_records;
            auto parse_start = std::chrono::high_resolution_clock::now();
            
            parser.parseFile(file_path.string(), 
                [&current_file_records](const ParsedRecord& record) {
                    current_file_records.push_back(record);
                }
            );

            auto parse_end = std::chrono::high_resolution_clock::now();
            total_parsing_duration += (parse_end - parse_start);

            if (current_file_records.empty()) {
                std::cout << "  -> No valid records found. Skipped." << std::endl;
                continue; 
            }
            
            auto insert_start = std::chrono::high_resolution_clock::now();
            inserter.insert_data_stream(current_file_records);
            auto insert_end = std::chrono::high_resolution_clock::now();
            total_insertion_duration += (insert_end - insert_start);

            successful_files++;
        }

        std::cout << "\nAll files processed successfully. Committing changes to the database..." << std::endl;
        inserter.commit_transaction();
        transaction_active = false;

        failed_files = files_to_process.size() - successful_files;

        std::cout << "\n----------------------------------------\n";
        std::cout << "Import process finished successfully.\n\n";
        std::cout << colors.green << "Successfully " << colors.reset <<  "processed files: " << successful_files << std::endl;
        if(failed_files > 0) std::cout << "Skipped empty/invalid files: " << failed_files << std::endl;
        std::cout << "----------------------------------------\n";
        
        std::cout << "Timing Statistics:\n\n";

        auto total_duration = total_parsing_duration + total_insertion_duration;
        double total_s = std::chrono::duration<double>(total_duration).count();
        double total_ms = std::chrono::duration<double, std::milli>(total_duration).count();
        double parsing_s = std::chrono::duration<double>(total_parsing_duration).count();
        double parsing_ms = std::chrono::duration<double, std::milli>(total_parsing_duration).count();
        double insertion_s = std::chrono::duration<double>(total_insertion_duration).count();
        double insertion_ms = std::chrono::duration<double, std::milli>(total_insertion_duration).count();

        std::cout << std::fixed;
        std::cout << "Total time: "
                  << std::setprecision(4) << total_s << " seconds ("
                  << std::setprecision(2) << total_ms << " ms)\n";
        std::cout << "Total text parsing time: "
                  << std::setprecision(4) << parsing_s << " seconds ("
                  << std::setprecision(2) << parsing_ms << " ms)\n";
        std::cout << "Total database insertion time: "
                  << std::setprecision(4) << insertion_s << " seconds ("
                  << std::setprecision(2) << insertion_ms << " ms)\n";
        std::cout << "----------------------------------------\n";

    } catch (const std::runtime_error& e) {
        if (transaction_active) {
            std::cerr << "\nAn error occurred. Rolling back all changes..." << std::endl;
            inserter.rollback_transaction();
        }
        
        std::cerr << "\n----------------------------------------\n";
        std::cerr << "Import process FAILED. No data was saved to the database.\n\n";
        std::cerr << "Error detail: " << e.what() << std::endl;
        std::cerr << "----------------------------------------\n";
    }
}

void handle_reporting_menu(int choice, const std::string& db_file) {
    BillReporter reporter(db_file);
    std::string year, category;

    switch (choice) {
        case 1:
            std::cout << "Enter year (e.g., 2025): ";
            std::cin >> year;
            reporter.query_1(year);
            break;
        case 2:
        case 3:
        { 
            std::string year_month_str;
            std::cout << "Enter year and month as a 6-digit number (e.g., 202501): ";
            std::cin >> year_month_str;

            if (year_month_str.length() != 6 || !std::all_of(year_month_str.begin(), year_month_str.end(), ::isdigit)) {
                std::cerr << "Error: Invalid format. Please enter exactly 6 digits." << std::endl;
            } else {
                std::string input_year = year_month_str.substr(0, 4);
                std::string input_month = year_month_str.substr(4, 2);
                if (choice == 2) {
                    reporter.query_2(input_year, input_month);
                } else {
                    reporter.query_3(input_year, input_month);
                }
            }
            break;
        }
        case 4:
            std::cout << "Enter year (e.g., 2025): ";
            std::cin >> year;
            std::cout << "Enter parent category name (e.g., MEAL吃饭): ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, category);
            reporter.query_4(year, category);
            break;
    }
}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif

    const std::string db_file = "bills.db";
    int choice = -1;

    while (choice != 5) {
        show_menu();
        std::cin >> choice;

        if (std::cin.fail() || choice < 0 || choice > 5) {
            std::cout << "Invalid input. Please enter a number between 0 and 5.\n";
            clear_cin();
            choice = -1; 
            continue;
        }

        try {
            if (choice == 0) {
                handle_import_process(db_file);
            } else if (choice > 0 && choice < 5) {
                handle_reporting_menu(choice, db_file);
            } else if (choice == 5) {
                std::cout << "Exiting program. Wish you a happy day!\n";
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "\nAn error occurred: " << e.what() << std::endl;
        }
    }

    return 0;
}