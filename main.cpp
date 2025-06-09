#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>
#include <filesystem> // For C++17 file system operations
#include <chrono>     // For high-precision timing
#include <iomanip>    // For std::setprecision
#include <algorithm>  // For std::all_of
#include <cctype>     // For ::isdigit


// Add the Windows header for console functions.
#ifdef _WIN32
#include <windows.h>
#endif

#include "BillParser.h"
#include "DatabaseInserter.h"
#include "BillReporter.h"

// C++17 filesystem alias
namespace fs = std::filesystem;

// Function to display the menu to the user
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

// Function to clear input buffer in case of invalid input
void clear_cin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

/**
 * @brief Handles the import process by prompting the user for a path.
 *
 * Prompts the user for a path to a .txt file or a directory. If a directory
 * is given, it recursively finds and processes all .txt files within it.
 * @param db_file The path to the SQLite database file.
 */
void handle_import_process(const std::string& db_file) {
    std::string user_path_str;
    std::cout << "Enter the path to a .txt file or a directory: ";

    // Clear the input buffer to handle the newline left by `std::cin >> choice`
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, user_path_str);

    fs::path user_path(user_path_str);
    std::vector<fs::path> files_to_process;

    try {
        if (!fs::exists(user_path)) {
            std::cerr << "Error: Path does not exist: " << user_path.string() << std::endl;
            return;
        }

        // Check if the path is a single .txt file
        if (fs::is_regular_file(user_path)) {
            if (user_path.extension() == ".txt") {
                files_to_process.push_back(user_path);
            } else {
                std::cerr << "Error: The provided file is not a .txt file." << std::endl;
            }
        // Check if the path is a directory and find all .txt files in it
        } else if (fs::is_directory(user_path)) {
            // --- FIXED: Use recursive_directory_iterator to find files in subdirectories ---
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

    // Start high-precision timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Process all collected files
    try {
        BillParser parser;
        DatabaseInserter inserter(db_file);
        inserter.create_database(); // Ensure DB schema is ready

        for (const auto& file_path : files_to_process) {
            std::cout << "\n--- Processing file: " << file_path.string() << " ---" << std::endl;
            parser.reset(); // Reset parser state for each new file
            parser.parseFile(file_path.string());
            const auto& records = parser.getRecords();

            if (records.empty()) {
                std::cout << "No valid records found in file. Skipping." << std::endl;
                continue;
            }

            std::cout << "Parsed " << records.size() << " records. Inserting into database..." << std::endl;
            inserter.insert_data_stream(records);
        }
        std::cout << "\nImport process completed." << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "\nAn error occurred during import: " << e.what() << std::endl;
    }
    
    // Stop timer and report duration
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = end_time - start_time;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    double seconds = milliseconds / 1000.0;

    std::cout << "\n----------------------------------------\n";
    std::cout << "Total import and processing time: "
              << milliseconds << " ms ("
              << std::fixed << std::setprecision(3) << seconds << " s)\n";
    std::cout << "----------------------------------------\n";
}


int main() {
    // Set console output to UTF-8 on Windows to correctly display Chinese characters and prevent hanging.
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif

    const std::string db_file = "bills.db";
    int choice = -1;

    // The main loop continues until the user chooses to exit (5)
    while (choice != 5) {
        show_menu();
        std::cin >> choice;

        // Input validation
        if (std::cin.fail() || choice < 0 || choice > 5) {
            std::cout << "Invalid input. Please enter a number between 0 and 5.\n";
            clear_cin();
            choice = -1; // Reset choice to continue the loop
            continue;
        }

        try {
            if (choice == 0) {
                // New, user-driven import process
                handle_import_process(db_file);

            } else if (choice > 0 && choice < 5) {
                // For choices 1-4, we need the BillReporter
                BillReporter reporter(db_file);
                std::string year, category;

                switch (choice) {
                    case 1: // Annual summary
                        std::cout << "Enter year (e.g., 2025): ";
                        std::cin >> year;
                        reporter.query_1(year);
                        break;

                    case 2: // Detailed monthly bill
                    case 3: // Export monthly bill
                    { // Use a block to create variables with local scope
                        std::string year_month_str;
                        std::cout << "Enter year and month as a 6-digit number (e.g., 202501): ";
                        std::cin >> year_month_str;

                        // Validate input format
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

                    case 4: // Annual category statistics
                        std::cout << "Enter year (e.g., 2025): ";
                        std::cin >> year;
                        std::cout << "Enter parent category name (e.g., MEAL吃饭): ";
                        // Use getline after cin to read the whole category name, even with spaces
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::getline(std::cin, category);
                        reporter.query_4(year, category);

                        break;
                }
            } else if (choice == 5) {
                std::cout << "Exiting program. Goodbye!\n";
            }
        } catch (const std::runtime_error& e) {
            // Catch errors from BillReporter or DatabaseInserter (e.g., DB not found)
            std::cerr << "\nAn error occurred: " << e.what() << std::endl;
        }
    }

    return 0;
}