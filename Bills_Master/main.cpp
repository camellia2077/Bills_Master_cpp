#include <iostream>
#include <string>
#include <limits>

// 包含我们所有的功能模块接口
#include "Reprocessor.h"
#include "DataProcessor.h" 
#include "QueryDb.h" 

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
    std::cout << "1. Validate Bill File\n"; 
    std::cout << "2. Modify Bill File\n"; 
    std::cout << "3. Parse and Insert Bill to Database\n"; 
    std::cout << "4. Query Yearly Summary\n";
    std::cout << "5. Query Monthly Details\n";
    std::cout << "6. Exit\n";
    std::cout << "=================================\n"; 
    std::cout << "Enter your choice: "; 
}

int main() {
    setup_console(); 

    std::cout << "Welcome to the Bill Reprocessor! (UTF-8 enabled)\n"; 

    try {
        Reprocessor reprocessor("./config"); 
        DataProcessor data_processor; 

        int choice = 0;
        while (choice != 6) { 
            print_menu();
            std::cin >> choice;

            if (std::cin.fail()) { 
                std::cin.clear(); 
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
                std::cout << "Invalid input. Please enter a number.\n"; 
                choice = 0; 
                continue; 
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

            std::string bill_path; 

            switch (choice) {
                case 1: { 
                    std::cout << "Enter path to bill file for validation: "; 
                    std::getline(std::cin, bill_path); 
                    if (!bill_path.empty()) { 
                        reprocessor.validate_bill(bill_path); 
                    } else {
                        std::cout << "Bill file path cannot be empty.\n"; 
                    }
                    break; 
                }
                case 2: { 
                    std::cout << "Enter path to source bill file for modification: "; 
                    std::getline(std::cin, bill_path); 

                    if (bill_path.empty()) { 
                        std::cout << "Source bill file path cannot be empty.\n"; 
                        break; 
                    }

                    size_t last_slash_pos = bill_path.find_last_of("/\\"); 
                    std::string filename = (std::string::npos != last_slash_pos) ? bill_path.substr(last_slash_pos + 1) : bill_path; 
                    std::string output_path = "modified_" + filename; 

                    reprocessor.modify_bill(bill_path, output_path); 
                    break; 
                }
                case 3: { 
                    std::cout << "Enter path to bill file to parse: "; 
                    std::getline(std::cin, bill_path); 
                    
                    const std::string db_path = "bills.db"; 

                    if (!bill_path.empty()) { 
                        std::cout << "Using database file: " << db_path << "\n"; 
                        data_processor.process_and_insert(bill_path, db_path); 
                    } else {
                        std::cout << "Bill file path cannot be empty.\n"; 
                    }
                    break; 
                }
                
                // *** MODIFIED: 使用新的 QueryFacade ***
                case 4: {
                    std::string year;
                    std::cout << "Enter year to query (e.g., 2025): ";
                    std::getline(std::cin, year);
                    if (!year.empty()) {
                        try {
                            QueryFacade facade("bills.db");
                            facade.show_yearly_summary(year);
                        } catch (const std::runtime_error& e) {
                            std::cerr << "Query failed: " << e.what() << std::endl;
                        }
                    } else {
                        std::cout << "Year cannot be empty.\n";
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
                            facade.show_monthly_details(month);
                        } catch (const std::runtime_error& e) {
                            std::cerr << "Query failed: " << e.what() << std::endl;
                        }
                    } else {
                        std::cout << "Month cannot be empty.\n";
                    }
                    break;
                }
                case 6: 
                    std::cout << "Exiting program. Goodbye!\n";
                    break;
                default:
                    std::cout << "Invalid choice. Please select a number from the menu.\n"; 
                    break;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "\nA critical error occurred: " << e.what() << std::endl; 
        return 1; 
    }

    return 0; 
}