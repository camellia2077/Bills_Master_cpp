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
#include "Bill_Parser.h" // 包含新的解析器头文件
#include "Database_Inserter.h"
#include "Bill_Reporter.h"

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
    // Bill_Parser parser(validator); // This line is not needed as it's created inside the loop
    Colors colors;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (true) {
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
        
        // We create the parser here, once per validation batch
        Bill_Parser parser(validator);

        for (const auto& file_path : files_to_process) {
            // 新逻辑：调用parseFile并检查返回的错误列表
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

        if (all_files_in_batch_valid) {
             std::cout << "\n----------------------------------------\n";
             std::cout << colors.green << "All files in path '" << user_path_str << "' are valid." << colors.reset << std::endl;
             std::cout << "----------------------------------------\n";
        } else {
            std::cout << "\nValidation finished. Some files contained errors.\n";
        }
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
            
            // 新逻辑：调用新的解析器
            std::vector<std::string> errors = parser.parseFile(file_path.string(), 
                [&current_file_records](const ParsedRecord& record) {
                    current_file_records.push_back(record);
                }
            );

            // 对于导入操作，我们仍然坚持“一旦有错，整个文件作废”的原则
            if (!errors.empty()) {
                // 构造一个包含所有错误的详细信息并抛出异常，以回滚事务
                std::stringstream error_stream;
                error_stream << "Validation failed in file " << file_path.string() << " with " << errors.size() << " errors. Details:\n";
                for(const auto& err : errors) {
                    error_stream << "  - " << err << "\n";
                }
                throw std::runtime_error(error_stream.str());
            }

            if (current_file_records.empty()) {
                std::cout << "File " << file_path.string() << " has no records to import. Skipped." << std::endl;
                continue; 
            }
            
            inserter.insert_data_stream(current_file_records);
        }

        std::cout << "\nAll valid files processed successfully. Committing changes to the database..." << std::endl;
        inserter.commit_transaction();
        transaction_active = false;
        
        // ... (此处可以添加成功导入的统计信息) ...

    } catch (const std::runtime_error& e) {
        if (transaction_active) {
            std::cerr << "\nAn error occurred. Rolling back all changes..." << std::endl;
            inserter.rollback_transaction();
        }
        std::cerr << "\nImport process FAILED. No data was saved to the database.\nError details: " << e.what() << std::endl;
    }
}

void handle_reporting_menu(int choice, const std::string& db_file) {
    BillReporter reporter(db_file);
    std::string year, category;
    switch (choice) {
        case 2:
            std::cout << "Enter year (e.g., 2025): "; std::cin >> year;
            reporter.query_1(year);
            break;
        case 3:
        case 4: {
            std::string year_month_str;
            std::cout << "Enter year and month as a 6-digit number (e.g., 202501): ";
            std::cin >> year_month_str;
            if (year_month_str.length() != 6 || !std::all_of(year_month_str.begin(), year_month_str.end(), ::isdigit)) {
                std::cerr << "Error: Invalid format. Please enter exactly 6 digits." << std::endl;
            } else {
                if (choice == 3) reporter.query_2(year_month_str.substr(0, 4), year_month_str.substr(4, 2));
                else reporter.query_3(year_month_str.substr(0, 4), year_month_str.substr(4, 2));
            }
            break;
        }
        case 5:
            std::cout << "Enter year (e.g., 2025): "; std::cin >> year;
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
