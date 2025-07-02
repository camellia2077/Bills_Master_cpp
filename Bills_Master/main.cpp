// main.cpp
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
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Line_Validator.h"
#include "Bill_Parser.h" 
#include "Database_Inserter.h"
#include "Bill_Queries.h"


namespace fs = std::filesystem;
struct Colors {
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string reset = "\033[0m";
};

class TimingLineValidator : public LineValidator {
    private:
        mutable std::chrono::nanoseconds m_duration{0};
    public:
        explicit TimingLineValidator(const std::string& config_path)
            : LineValidator(config_path) {}
        ValidationResult validate(const std::string& line) const override {
            auto start = std::chrono::high_resolution_clock::now();
            ValidationResult result = LineValidator::validate(line);
            auto end = std::chrono::high_resolution_clock::now();
            m_duration += (end - start);
            return result;
        }
        std::chrono::nanoseconds get_duration() const { return m_duration; }
        void reset_duration() { m_duration = std::chrono::nanoseconds(0); }
};

void show_menu() {
    std::cout << "\n===== Bill Management System =====\n";
    std::cout << "0. Validate .txt file(s) (no import)\n";
    std::cout << "1. Import data from .txt file(s)\n";
    std::cout << "2. Annual consumption summary\n";
    std::cout << "3. Detailed monthly bill\n";
    std::cout << "4. Export monthly bill (machine-readable)\n";
    std::cout << "5. Annual category statistics\n";
    std::cout << "6. Exit\n";
    std::cout << "==================================\n";
    std::cout << "Enter your choice: ";
}

void clear_cin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 新增：用于一次性打印一个文件的所有错误
void print_all_errors_for_file(const std::string& file_path, const std::vector<std::string>& errors) {
    Colors colors;
    std::cerr << "\n----------------------------------------\n";
    std::cerr << colors.red << "Validation FAILED" << colors.reset << " in file: " << file_path << "\n\n";
    std::cerr << "Found " << errors.size() << " error(s):\n";
    for (const auto& err : errors) {
        std::cerr << "  - " << err << std::endl;
    }
    std::cerr << "----------------------------------------\n";
}

void handle_validation_only_process() {
    TimingLineValidator validator("Validator_Config.json");
    Colors colors;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (true) {
        validator.reset_duration();
        std::string user_path_str;
        std::cout << "\nEnter the path to a .txt file or directory to validate (or 'q' to return to menu): ";
        std::getline(std::cin, user_path_str);

        if (user_path_str == "q" || user_path_str == "Q") {
            std::cout << "Returning to main menu.\n";
            break;
        }

        fs::path user_path(user_path_str);
        std::vector<fs::path> files_to_process;

        try {
            if (!fs::exists(user_path)) {
                std::cerr << "Error: Path does not exist: " << user_path.string() << std::endl;
                continue;
            }
            if (fs::is_regular_file(user_path)) {
                if (user_path.extension() == ".txt") files_to_process.push_back(user_path);
                else { std::cerr << "Error: Provided file is not a .txt file." << std::endl; continue; }
            } else if (fs::is_directory(user_path)) {
                for (const auto& entry : fs::recursive_directory_iterator(user_path)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".txt")
                        files_to_process.push_back(entry.path());
                }
            } else { std::cerr << "Error: The provided path is not a regular file or directory." << std::endl; continue; }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl; continue;
        }

        if (files_to_process.empty()) {
            std::cout << "No .txt files found to process in the given path." << std::endl; continue;
        }

        bool all_files_in_batch_valid = true;
        std::cout << "\nStarting validation...\n";
        
        auto validation_start_time = std::chrono::high_resolution_clock::now();
        
        Bill_Parser parser(validator);

        for (const auto& file_path : files_to_process) {
            std::vector<std::string> errors = parser.parseFile(file_path.string(), [](const ParsedRecord& record){
                // 在仅验证模式下，我们不需要处理有效记录
            });

            if (errors.empty()) {
                std::cout << "  " << colors.green << "[VALID] " << colors.reset << file_path.string() << std::endl;
            } else {
                print_all_errors_for_file(file_path.string(), errors);
                all_files_in_batch_valid = false;
            }
        }
        
        auto validation_end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = validation_end_time - validation_start_time;

        if (all_files_in_batch_valid) {
             std::cout << "\n----------------------------------------\n";
             std::cout << colors.green << "All files in path '" << user_path_str << "' are valid." << colors.reset << std::endl;
             std::cout << "----------------------------------------\n";
        } else {
            std::cout << "\nValidation finished. Some files contained errors.\n";
        }

        auto pure_validation_duration = validator.get_duration();
        auto overhead_duration = total_duration - pure_validation_duration;
        if (overhead_duration.count() < 0) {
            overhead_duration = std::chrono::nanoseconds(0);
        }

        double total_sec = std::chrono::duration<double>(total_duration).count();
        double total_ms = std::chrono::duration<double, std::milli>(total_duration).count();
        double pure_val_sec = std::chrono::duration<double>(pure_validation_duration).count();
        double pure_val_ms = std::chrono::duration<double, std::milli>(pure_validation_duration).count();
        double overhead_sec = std::chrono::duration<double>(overhead_duration).count();
        double overhead_ms = std::chrono::duration<double, std::milli>(overhead_duration).count();

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "\n--------------------------------------\n";
        std::cout << "Validation Timing Statistics:\n\n";
        std::cout << "Total time: " << total_sec << " seconds (" << total_ms << " ms)\n";
        std::cout << "  - Pure line validation: " << pure_val_sec << " seconds (" << pure_val_ms << " ms)\n";
        std::cout << "  - File I/O & parsing overhead: " << overhead_sec << " seconds (" << overhead_ms << " ms)\n";
        std::cout << "--------------------------------------\n";
    }
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
        if (!fs::exists(user_path)) { std::cerr << "Error: Path does not exist: " << user_path.string() << std::endl; return; }
        if (fs::is_regular_file(user_path)) {
            if (user_path.extension() == ".txt") files_to_process.push_back(user_path);
            else { std::cerr << "Error: The provided file is not a .txt file." << std::endl; return; }
        } else if (fs::is_directory(user_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(user_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".txt") files_to_process.push_back(entry.path());
            }
        } else { std::cerr << "Error: The provided path is not a regular file or directory." << std::endl; return; }
    } catch (const fs::filesystem_error& e) { std::cerr << "Filesystem error: " << e.what() << std::endl; return; }

    if (files_to_process.empty()) { std::cout << "No .txt files found to process." << std::endl; return; }

    auto total_start_time = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds parsing_duration{0};
    std::chrono::nanoseconds db_insertion_duration{0};

    DatabaseInserter inserter(db_file);
    TimingLineValidator validator("Validator_Config.json");
    Bill_Parser parser(validator);
    bool transaction_active = false;

    try {
        inserter.create_database();
        inserter.begin_transaction();
        transaction_active = true;

        for (const auto& file_path : files_to_process) {
            std::vector<ParsedRecord> current_file_records;
            
            auto parsing_start = std::chrono::high_resolution_clock::now();
            std::vector<std::string> errors = parser.parseFile(file_path.string(),
                [&current_file_records](const ParsedRecord& record) {
                    current_file_records.push_back(record);
                }
            );
            auto parsing_end = std::chrono::high_resolution_clock::now();
            parsing_duration += (parsing_end - parsing_start);

            if (!errors.empty()) {
                std::cout << colors.yellow << "Warning: " << colors.reset << "File " << file_path.string()
                          << " contains " << errors.size() << " errors. Only valid lines will be imported." << std::endl;
            }

            if (current_file_records.empty()) {
                std::cout << "File " << file_path.string() << " has no valid records to import. Skipped." << std::endl;
                continue; 
            }
            
            auto db_start = std::chrono::high_resolution_clock::now();
            inserter.insert_data_stream(current_file_records);
            auto db_end = std::chrono::high_resolution_clock::now();
            db_insertion_duration += (db_end - db_start);
        }

        std::cout << "\nAll processable files attempted. Committing changes to the database..." << std::endl;
        inserter.commit_transaction();
        transaction_active = false;
        std::cout << colors.green << "Successfully inserted data into the database." << colors.reset << std::endl;
        
        auto total_end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = total_end_time - total_start_time;

        double total_sec = std::chrono::duration<double>(total_duration).count();
        double total_ms = std::chrono::duration<double, std::milli>(total_duration).count();
        double parsing_sec = std::chrono::duration<double>(parsing_duration).count();
        double parsing_ms = std::chrono::duration<double, std::milli>(parsing_duration).count();
        double db_sec = std::chrono::duration<double>(db_insertion_duration).count();
        double db_ms = std::chrono::duration<double, std::milli>(db_insertion_duration).count();

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "\n--------------------------------------\n";
        std::cout << "Timing Statistics:\n\n";
        std::cout << "Total time: " << total_sec << " seconds (" << total_ms << " ms)\n";
        std::cout << "  - Parsing files: " << parsing_sec << " seconds (" << parsing_ms << " ms)\n";
        std::cout << "  - Database insertion: " << db_sec << " seconds (" << db_ms << " ms)\n";
        std::cout << "--------------------------------------\n";

    } catch (const std::runtime_error& e) {
        if (transaction_active) {
            std::cerr << "\nA critical error occurred. Rolling back all changes..." << std::endl;
            inserter.rollback_transaction();
        }
        std::cerr << "\nImport process FAILED. No data was saved to the database.\nError details: " << e.what() << std::endl;
    }
}

void handle_reporting_menu(int choice, const std::string& db_file) {
    BillQueries queries(db_file);
    std::string year, category;
    switch (choice) {
        case 2:
            std::cout << "Enter year (e.g., 2025): "; std::cin >> year;
            queries.query_1(year);
            break;
        case 3:
        case 4: {
            std::string year_month_str;
            std::cout << "Enter year and month as a 6-digit number (e.g., 202501): ";
            std::cin >> year_month_str;
            if (year_month_str.length() != 6 || !std::all_of(year_month_str.begin(), year_month_str.end(), ::isdigit)) {
                std::cerr << "Error: Invalid format. Please enter exactly 6 digits." << std::endl;
            } else {
                if (choice == 3) queries.query_2(year_month_str.substr(0, 4), year_month_str.substr(4, 2));
                else queries.query_3(year_month_str.substr(0, 4), year_month_str.substr(4, 2));
            }
            break;
        }
        case 5:
            std::cout << "Enter year (e.g., 2025): "; std::cin >> year;
            std::cout << "Enter parent category name (e.g., MEAL吃饭): ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, category);
            queries.query_4(year, category);
            break;
    }
}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif

    const std::string db_file = "bills.db";
    int choice = -1;

    while (choice != 6) {
        show_menu();
        std::cin >> choice;

        if (std::cin.fail() || choice < 0 || choice > 6) {
            std::cout << "Invalid input. Please enter a number between 0 and 6.\n";
            clear_cin();
            choice = -1; 
            continue;
        }

        try {
            if (choice == 0) {
                handle_validation_only_process();
            } else if (choice == 1) {
                handle_import_process(db_file);
            } else if (choice > 1 && choice < 6) {
                handle_reporting_menu(choice, db_file);
            } else if (choice == 6) {
                std::cout << "Exiting program. Wish you a happy day!\n";
            }
        } catch (const std::runtime_error& e) {
            std::string error_msg = e.what();
            if (error_msg.find("Could not open configuration file") != std::string::npos) {
                Colors colors;
                std::cerr << "\n" << colors.yellow << "Warning: " << colors.reset 
                          << "The configuration file 'Validator_Config.json' was not found.\n"
                          << "This file must be in the same directory as the executable program.\n" << std::endl;
            } else {
                std::cerr << "\nAn unexpected error occurred in the main loop: " << e.what() << std::endl;
            }
        }
    }

    return 0;
}